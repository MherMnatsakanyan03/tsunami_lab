10 OpenCL
=========

Links:
------------

`Github Repo <https://github.com/MherMnatsakanyan03/tsunami_lab.git>`_


Individual Contributions:
-------------------------

Mher Mnatsakanyan and Maurice Herold did a similar amount of work.


Project Plan
------------

Goals
^^^^^

The primary goal of this project is to enhance the performance of our tsunami simulation by integrating OpenCL for GPU-acceleration. This will involve parallelizing the :code:`timeStep` function in WavePropagation2d.cpp, which currently runs only on the CPU. By leveraging the power of GPU, we expect to achieve significant performance gains.

Our private Setup consists of an Intel i5 14600KF with a Radeon Rx 6700XT. Since the Chips themselves are very powerful, we expect the memory bandwidth to be the limiting factor.

* `Intel i5 14600KF`: :math:`89.6 GB/s`
* `Radeon Rx 6700XT`: :math:`384 GB/s`

This alone is a 4.3x speedup. But considering that the CPU is also used for other tasks, such as running the operating system itself, we expect the speedup to be even higher.

On the other hand, the GPU has a much higher latency than the CPU and there will be a overhead for copying the data to the GPU.

The exact amount of improvement will depend on the size of the simulation. For smaller simulations, the overhead of copying the data to the GPU will be significant, and the speedup will be minimal. However, for larger simulations, the speedup will be much more noticeable.

To measure the performance gains, we will use tools like the Radeon GPU Profiler and Intel VTune Amplifier. These tools will allow us to measure the execution time of the :code:`timeStep` function before and after the integration of OpenCL.

Milestones & Time Schedule
^^^^^^^^^^^^^^^^^^^^^^^^^^

The project will be completed over a period of four weeks, starting from the date of this plan. Each week will be dedicated to a specific milestone, ensuring that the project stays on track and is completed within the given timeframe.

Each milestone represents a separate work package, with specific tasks to be completed within the given week. The tasks will be divided equally among the team members, ensuring that everyone contributes to the project's success.

1. **Week 1 + 2: Writing the Kernel**

* The first two weeks will be the most important. They will be dedicated to writing the OpenCL kernel
* Considering that the for-loops are completely independant, we expect that we can easily vectorize the data
* The detailed implementation will obviously depend on what we learn over the course of the project, but we think that the kernel will mainly consist of the calculations done in the FWave-solver, which will then be applied to each cell in parallel instead of sequentially going through for-loop iterations


2. **Week 3: Kernel Integration** 

* In the third week, we will integrate the newly written kernel into the existing tsunami simulation project.
* This will involve modifying the WavePropagation2d.cpp file and ensuring that the kernel is correctly utilized.
* We plan on adding a parameter that allows the user to specify whether the simulation should be run on the CPU or GPU.

3. **Week 4: Performance Testing**

* The final week will be spent testing the performance gains achieved by the GPU-acceleration. The main metric will be the final time that the simulation takes before finishing, but we also plan on using tools like the Radeon GPU Profiler to measure the execution time of the :code:`timeStep` function before and after the integration of OpenCL.

Project Report
--------------

Throughout the project, we encountered several challenges that we had to overcome.
The first issue was to undestand how to run OpenCL and try to build some small kernels
to get a feeling for the syntax. This was a huge deal since the documatation on how to
do so is very sparse and very different from different sources.

Week 1:
^^^^^^^

First we tried to compile OpenCL on Linux using WSL2, but it took some time for us to
we realized that this would not work since WSL2 does not support GPU acceleration via OpenCL.
We then switched to Windows and tried to compile OpenCL. This worked after a lot of fidling
with MSYS2 and the compiler-settings. Not-so-fun-fact: The OpenCL-headers need to be downloaded,
which is not mentioned anywhere if you are trying to look for it on AMD-documentation. Instead,
we had to rely on Nvidia-documentation to get OpenCL to compile on our AMD-GPU.

After getting OpenCL to compile, we tried to add Windows support to the project. This turned
out to be a huge waste of time, because after setting up the simulation to run on Windows and
the compiler not throwing errors, the executable was complaining about missing DLLs. We tried
tried a lot of things but ended up giving up on Windows-support, because of the following:

.. image:: _static/content/images/Week10/Windows_regular_compile.png

This is the error message we recieved when trying to execute the programm. After countless attempts
and research done, we tried to add to copy the DLL into the same folder as the executable.

.. image:: _static/content/images/Week10/Windows_adding_libnetcdf.png

Windows told us, that now a different DLL was missing. Adding this one too threw a new error
stating another DLL was missing. After adding all of the DLLs that are in the `mingw64/bin` folder,
we got the following error:

.. image:: _static/content/images/Week10/Windows_Adding_all_dlls.png

At this point we gave up on Windows-support, since we were not able to find a solution to this problem.

Week 2:
^^^^^^^

After a lot more reasearch was done, we found pocl, which allowed us to run OpenCL on the CPU. This
was great, since now we were able to run OpenCL on WSL, so the development could continue on Linux.
Fun-fact: since we assumed that OpenCL would not work on Apple-Silicon, we were pleasantly surprised
when OpenCL worked on our M2-Macbook. In fact, the integration of OpenCL was much easier on the Mac
and did not require any workarounds.

At this point we were also developing the main kernel, which we were able to test now inside the simulation.
At first we wrote a bad implementation, which was loading the data from the global memory for every
calculation. This was very slow and we were not able to get any speedup, but we were happy that at least
we made some progress.

Afterwards we tried to optimize the kernel by loading the data into the local memory. This was a huge
success for the performance, since for the first time we were able to compile faster than the CPU (on the Mac).
Our happy moment was cut short after realizing that the results were not correct. After some debugging,
we were able to get the simulation to run without throwing nans and infinities, but the results were still
bad:

|Dambreak_CPU| |Dambreak_OpenCL|

.. |Dambreak_CPU| image:: _static/content/images/Week10/Dambreak_CPU.png
   :width: 45%

.. |Dambreak_OpenCL| image:: _static/content/images/Week10/Dambreak_OpenCL.png
   :width: 45%

Left: CPU, Right: OpenCL

Week 3:
^^^^^^^

At this point, we were nervous that we would not be able to get the kernel to work correctly. We tried
a lot of thing, but ended up not finding the problem. Thats when Justus came in clutch and helped us.
He showed us that his implementation with CUDA did something automatically, which OpenCL did not do.
The problem was, that we had a synchronization problem. The kernel was not waiting for all threads to
finish, which lead to the x-sweep and y-sweep to be executed at the same time. This caused the issue
that the x-sweep was overwriting the results of the y-sweep. The following image illustrates the problem:

.. image:: _static/content/images/Week10/Illustration.png

This Illustration shows, that the left side is fine because the x-Sweep is executed first and then the
y-Sweep. The right side shows the problem, where the x-Sweep is executed at the same time as the y-Sweep.
This is a problem, because each sweep needs a constant state of the simulation, since it is adding values
to the same array. If the x-Sweep is executed at the same time as the y-Sweep, the x-Sweep will write
values to the array, where the y-Sweep writes values on the same array, but in different locations,
creating an unstable state. This is why in the results of last week, the simulation has that distorted
wave at the top right corner.

To fix this, we needed to split our big WavePropagation2d-kernel into four smaller kernels.

- The first kernel sets the Ghost-cells and the boundary conditions
- The second kernel copies the data from the array to a temporary array
- The third kernel executes the x-Sweep
- The fourth kernel executes the y-Sweep

Justus helped us to split the kernel into four smaller kernels, which is why we included him
as a co-contributor in the :code:`kernel.cl` file.
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

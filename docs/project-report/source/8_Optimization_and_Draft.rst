8 Optimization and Idea-Draft for individual part
=================================================

Links:
------------

`Github Repo <https://github.com/MherMnatsakanyan03/tsunami_lab.git>`_


Individual Contributions:
-------------------------

Mher Mnatsakanyan and Maurice Herold did a similar amount of work.


OpenCL Implementation - Idea Draft
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

We're planning to use OpenCL to accelerate our calculations. OpenCL (Open Computing Language)
is a framework for writing programs that execute across heterogeneous platforms consisting of
CPU's, GPUs, digital signal processors (DSPs), field-programmable gate arrays (FPGAs) and other
processors or hardware accelerators.

In our current code, we're using OpenMP for parallel processing. However, by transitioning to OpenCL,
we can offload some of the heavy computations to the GPU, which can handle thousands of threads
simultaneously, leading to significant performance improvements.

Here's a simplified example of how we might modify our code to use OpenCL:

.. code:: cpp

    // Create an OpenCL context
    cl::Context context({default_device});

    // Create a command queue
    cl::CommandQueue queue(context, default_device);

    // Read and compile the kernel code
    std::ifstream sourceFile("path_to_your_kernel.cl");
    std::string sourceCode(std::istreambuf_iterator<char>(sourceFile), (std::istreambuf_iterator<char>()));
    cl::Program::Sources source(1, std::make_pair(sourceCode.c_str(), sourceCode.length() + 1));
    cl::Program program(context, source);

    // Build the kernel
    program.build({default_device});

    // Create the kernel
    cl::Kernel kernel(program, "your_kernel_name");

    // Create memory buffers
    cl::Buffer buffer_A(context, CL_MEM_READ_ONLY, size * sizeof(int));
    cl::Buffer buffer_B(context, CL_MEM_READ_ONLY, size * sizeof(int));
    cl::Buffer buffer_C(context, CL_MEM_WRITE_ONLY, size * sizeof(int));

    // Copy lists A and B to the memory buffers
    queue.enqueueWriteBuffer(buffer_A, CL_TRUE, 0, size * sizeof(int), A);
    queue.enqueueWriteBuffer(buffer_B, CL_TRUE, 0, size * sizeof(int), B);

    // Set arguments to the kernel
    kernel.setArg(0, buffer_A);
    kernel.setArg(1, buffer_B);
    kernel.setArg(2, buffer_C);

    // Run the kernel
    queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(size), cl::NullRange);

    // Read buffer C into a local list
    int* C = new int[size];
    queue.enqueueReadBuffer(buffer_C, CL_TRUE, 0, size * sizeof(int), C);

This is a basic example and doesn't include error checking or optimization, but it should give
you an idea of how we can use OpenCL to accelerate our tsunami simulations. The actual implementation
will depend on the specifics of our project and the architecture of our hardware. We'll also
need to write the OpenCL kernel code, which will perform the actual computations on the GPU.
This code will be similar to our current computation code, but it will be designed to run efficiently
on a GPU. 


8.1: ARA-cluster
----------------

8.1.3: ARA-cluster vs private Computer
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The simulations on the ARA-cluster ran significantly slower than on our private computers.
This is due to our private testing-computers having a better single-core performance than the
ARA-cluster. The ARA-cluster has a better multi-core performance, but we removed the OpenMP
to test the single-core performance.

The CPU tested on was an Intel Core i5-14600KF with a boost clock of 5.3 GHz.

Here are the results of the tests (O2-optimization):

.. code:: python

    # 14600KF
    simulation time / #time steps: 34315.6 / 7000
        Time since programm started: 63.5221s

    # ARA-cluster
    simulation time / #time steps: 34315.6 / 7000
        Time since programm started: 529.915s


8.2: Compilers
--------------

8.2.1: Option
^^^^^^^^^^^^^

We added a new option to SCONS, which allows us to choose the compiler we want to use.
The implementation looks like this:

.. code:: python

    # set compiler
    cxx_compiler = ARGUMENTS.get('comp', "g++")

    if cxx_compiler == 'g++':
        pass
    else:
        env['CXX'] = "/opt/intel/oneapi/compiler/2023.2.2/linux/bin/intel64/icpc"
        env.Append( CXXFLAGS = ['-qopt-report=5'])

8.2.2: Perfomance comparison
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The Intel compiler is a little slower, than the g++ compiler. The following results are
done using the Ofast-optimization.:

.. code:: python

    # g++
    simulation time / #time steps: 34315.6 / 7000
        Time to write: 0.0502338s
        Time since programm started: 388.847s

    # icpc
    simulation time / #time steps: 34315.6 / 7000
        Time to write: 0.0332013s
        Time since programm started: 408.733s


8.2.3: Optimization:
^^^^^^^^^^^^^^^^^^^^

We tried using the Intel compiler with the Ofast-optimization and we succeded.
The issue is that after a certain amount of time, the Intel compiler stoped
working and we weren't able to collect more benchmark data. The following
is the error-message we got:

.. code::

    Error: A license for Comp-CL could not be obtained.  (-1,359,2).

    Is your license file in the right location and readable?
    The location of your license file should be specified via
    the $INTEL_LICENSE_FILE environment variable.

We are not sure, why this error occured.

Regardless, we did test the g++ compiler with different settings and the 
results are as follows:

.. code:: python

    # O2
    simulation time / #time steps: 34315.6 / 7000
        Time to write: 0.0411987s
        Time since programm started: 529.915s
    # O3
    simulation time / #time steps: 34315.6 / 7000
        Time to write: 0.0348658s
        Time since programm started: 576.218s
    # Ofast
    simulation time / #time steps: 34315.6 / 7000
        Time to write: 0.0502338s
        Time since programm started: 388.847s


As you can see, the O3-optimization is suprisingly slow in comparison to the rest.

The Ofast-optimization is the fastest, but it is also the most dangerous, because
it'll freely reorder floating-point computations (which is prohibited by default because
in general a + (b + c) != (a + b) + c != a + (c + b) for floats).

Luckily, we didn't have any issues with the Ofast-optimization and it ran just fine.

8.2.4: -qopt-report
^^^^^^^^^^^^^^^^^^^

We tried using the -qopt-report flag to get a report on the optimizations icpc did,
but unfortunately we forgot to put that flag into the SCONS-file for the first run
and since the Intel compiler stopped working after a certain amount of time (see 8.2.3),
we weren't able to collect the additional report file.


8.3: Optimization:
------------------

8.3.1 - 8.3.3:
^^^^^^^^^^^^^^

We were not able to run Intel VTune unfortunately.

8.3.4: Intensive parts
^^^^^^^^^^^^^^^^^^^^^^

As expected, the most intensive parts of our code are the for-loops in `Wavepropagation2d.cpp`, where
we copy long arrays and also do some calculations. We also found that the `write_to_file` function
is quite intensive, but we're not sure if we can optimize it further, since we are using external
libraries.

8.3.5: Optimization
^^^^^^^^^^^^^^^^^^^^

We were able to find ways to make our code run significantly faster.
We realized, that the for-loops in `Wavepropagation2d.cpp` were running in an
inefficient way. We were able to fix this by changing the for-loops to the following:

.. code:: c++

    // old
    for (t_idx l_x = 1; l_x < m_nCells_x + 1; l_x++)
    {
        // #pragma omp parallel for
        for (t_idx l_y = 1; l_y < m_nCells_y + 1; l_y++)
        {

    // new
    for (t_idx l_y = 1; l_y < m_nCells_y + 1; l_y++)
    {
        // #pragma omp parallel for
        for (t_idx l_x = 1; l_x < m_nCells_x + 1; l_x++)
        {

This change was due to the fact that the for-loops were running in a column-major
way before, which made it so that each array needed to be loaded again for each
iteration of the inner for-loop. By changing the for-loops to run in a row-major
way, we were able to significantly reduce the amount of times the arrays needed
to be loaded.

.. code:: python

    # original
    simulation time / #time steps: 35878.6 / 11000
        Time since programm started: 528.046s
    # Optimized x-sweep
    simulation time / #time steps: 35878.6 / 11000
        Time since programm started: 424.735s
    # Optimized y-sweep
    simulation time / #time steps: 35878.6 / 11000
        Time since programm started: 368.706s
    # Optimized x-sweep + y-sweep
    simulation time / #time steps: 35878.6 / 11000
        Time since programm started: 337.627s
    #Optimized just copy-loops
    simulation time / #time steps: 35878.6 / 11000
        Time since programm started: 326.54s
    # all optimizations
    simulation time / #time steps: 35878.6 / 11000
        Time since programm started: 218.302s


This means that we were able to reduce the runtime by 58.9% by just changing the
for-loops to run in a row-major way.

Spoiler for next week: OpenMP ran even smoother than that:

.. code:: python

    # all optimizations + OpenMP
    simulation time / #time steps: 35878.6 / 11000
        Time since programm started: 27.8955s
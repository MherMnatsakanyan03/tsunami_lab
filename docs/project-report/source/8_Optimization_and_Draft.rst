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

/**
 * @author Maurice Herold (maurice.herold AT uni-jena.de)
 * @author Mher Mnatsakanyan (mher.mnatsakanyan AT uni-jena.de)
 *
 * @section DESCRIPTION
 * Two-dimensional wave propagation patch.
 **/

#define CL_TARGET_OPENCL_VERSION 300
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#define PROGRAM_FILE "/Users/ibyton/Desktop/Uni/tsunami_lab/build/src/patches/wavepropagation2d_kernel/kernel.cl"
// #define PROGRAM_FILE "kernel.cl"
#define KERNEL_X_AXIS_FUNC "updateXAxisKernel"
#define KERNEL_Y_AXIS_FUNC "updateYAxisKernel"
#define KERNEL_UPDATE_CELLS "updateCellsKernel"

#include "WavePropagation2d_kernel.h"

#include <iostream>
#include <stdexcept>
#include <string>
// #include <CL/cl.h>
#include "../../solvers/f-wave/F_wave.h"

cl_device_id create_device()
{

    cl_platform_id platform;
    cl_device_id dev;
    int err;

    /* Identify a platform */
    err = clGetPlatformIDs(1, &platform, NULL);
    if (err < 0)
    {
        perror("Couldn't identify a platform");
        exit(1);
    }

    // Access a device
    // GPU
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &dev, NULL);
    if (err == CL_DEVICE_NOT_FOUND)
    {
        // CPU
        err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &dev, NULL);
    }
    if (err < 0)
    {
        perror("Couldn't access any devices");
        exit(1);
    }

    return dev;
}

cl_program build_program(cl_context ctx, cl_device_id dev, const char *filename)

{

    cl_program program;
    FILE *program_handle;
    char *program_buffer, *program_log;
    size_t program_size, log_size;
    int err;

    program_handle = fopen(filename, "rb");
    if (program_handle == NULL)
    {
        perror("Couldn't find the program file");
        exit(1);
    }
    fseek(program_handle, 0, SEEK_END);
    program_size = ftell(program_handle);
    rewind(program_handle);
    program_buffer = (char *)malloc(program_size + 1);
    program_buffer[program_size] = '\0';
    size_t items_read = fread(program_buffer, sizeof(char), program_size, program_handle);
    if (items_read != program_size)
    {
        exit(1);
    }
    fclose(program_handle);

    program = clCreateProgramWithSource(ctx, 1,
                                        (const char **)&program_buffer, &program_size, &err);
    if (err < 0)
    {
        perror("Couldn't create the program");
        exit(1);
    }
    free(program_buffer);

    err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if (err < 0)
    {

        /* Find size of log and print to std output */
        clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG,
                              0, NULL, &log_size);
        program_log = (char *)malloc(log_size + 1);
        program_log[log_size] = '\0';
        clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG,
                              log_size + 1, program_log, NULL);
        printf("%s\n", program_log);
        free(program_log);
        exit(1);
    }

    return program;
}

size_t findMaxLocalSize(cl_device_id device)
{
    size_t maxLocalSize;
    cl_int err;

    // Abfrage der maximalen Arbeitsgruppengröße
    err = clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(maxLocalSize), &maxLocalSize, NULL);
    if (err != CL_SUCCESS)
    {
        // Fehlerbehandlung
        std::cerr << "Error: Could not retrieve maximum work group size." << std::endl;
        exit(1);
    }

    return maxLocalSize;
}

tsunami_lab::patches::WavePropagation2d_kernel::WavePropagation2d_kernel(t_idx i_nCells_x,
                                                                         t_idx i_nCells_y,
                                                                         int state_boundary_left,
                                                                         int state_boundary_right,
                                                                         int state_boundary_top,
                                                                         int state_boundary_bottom)
{
    m_nCells_x = i_nCells_x;
    m_nCells_y = i_nCells_y;
    m_state_boundary_left = state_boundary_left;
    m_state_boundary_right = state_boundary_right;
    m_state_boundary_top = state_boundary_top;
    m_state_boundary_bottom = state_boundary_bottom;

    // allocate memory including a single ghost cell on each side and initializing with 0
    // The 2d x-y grid is being flattened into a 1d array
    m_h = new t_real[(m_nCells_x + 2) * (m_nCells_y + 2)]{0};
    m_hu = new t_real[(m_nCells_x + 2) * (m_nCells_y + 2)]{0};
    m_hv = new t_real[(m_nCells_x + 2) * (m_nCells_y + 2)]{0};

    m_b = new t_real[(m_nCells_x + 2) * (m_nCells_y + 2)]{0};

    device = create_device();

    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);

    program = build_program(context, device, PROGRAM_FILE);

    m_h_buff = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(float) * (m_nCells_x + 2) * (m_nCells_y + 2), NULL, &err);
    m_hu_buff = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(float) * (m_nCells_x + 2) * (m_nCells_y + 2), NULL, &err);
    m_hv_buff = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(float) * (m_nCells_x + 2) * (m_nCells_y + 2), NULL, &err);
    m_b_buff = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(float) * (m_nCells_x + 2) * (m_nCells_y + 2), NULL, &err);
    m_hNew_buff = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(float) * (m_nCells_x + 2) * (m_nCells_y + 2), NULL, &err);
    m_huNew_buff = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(float) * (m_nCells_x + 2) * (m_nCells_y + 2), NULL, &err);
    m_hvNew_buff = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(float) * (m_nCells_x + 2) * (m_nCells_y + 2), NULL, &err);
}

tsunami_lab::patches::WavePropagation2d_kernel::~WavePropagation2d_kernel()
{

    delete[] m_h;
    delete[] m_hu;
    delete[] m_hv;

    delete[] m_b;

    clReleaseProgram(program);
    clReleaseContext(context);
    clReleaseMemObject(m_h_buff);
    clReleaseMemObject(m_hu_buff);
    clReleaseMemObject(m_hv_buff);
    clReleaseMemObject(m_b_buff);
}

void tsunami_lab::patches::WavePropagation2d_kernel::timeStep(t_real i_scaling)
{
    //
    // X-AXIS
    //

    setGhostOutflow();

    // Create kernel
    kernel = clCreateKernel(program, KERNEL_X_AXIS_FUNC, &err);

    // Create a command queue
    queue = clCreateCommandQueue(context, device, 0, &err);

    // Create memory buffers

    // Set kernel arguments
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &m_hNew_buff);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &m_huNew_buff);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), &m_hvNew_buff);
    clSetKernelArg(kernel, 3, sizeof(cl_mem), &m_b_buff);
    clSetKernelArg(kernel, 4, sizeof(int), &m_nCells_x);
    clSetKernelArg(kernel, 5, sizeof(int), &m_nCells_y);
    clSetKernelArg(kernel, 6, sizeof(float), &i_scaling);
    clSetKernelArg(kernel, 7, sizeof(cl_mem), &m_h_buff);
    clSetKernelArg(kernel, 8, sizeof(cl_mem), &m_hu_buff);

    size_t maxLocalSize = findMaxLocalSize(device);

    // Enqueue kernel
    size_t global_size[2] = {m_nCells_x + 2, m_nCells_y + 2}; // Gesamtanzahl der Work-Items in X- und Y-Richtung
    size_t local_size[2] = {maxLocalSize / 2, maxLocalSize / 2};

    clEnqueueNDRangeKernel(queue, kernel, 2, NULL, global_size, local_size, 0, NULL, NULL);
    clFinish(queue);

    // Read the output data back to the host
    clEnqueueReadBuffer(queue, m_h_buff, CL_TRUE, 0, sizeof(float) * (m_nCells_x + 2) * (m_nCells_y + 2), m_h, 0, NULL, NULL);
    clEnqueueReadBuffer(queue, m_hu_buff, CL_TRUE, 0, sizeof(float) * (m_nCells_x + 2) * (m_nCells_y + 2), m_hu, 0, NULL, NULL);

    clReleaseKernel(kernel);
    clReleaseCommandQueue(queue);

    //
    // Y-AXIS
    //
    setGhostOutflow();
    // iterate over edges and update with Riemann solutions in y-direction
    // Create kernel
    kernel = clCreateKernel(program, KERNEL_Y_AXIS_FUNC, &err);

    // Create a command queue
    queue = clCreateCommandQueue(context, device, 0, &err);

    // Set kernel arguments
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &m_hNew_buff);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &m_huNew_buff);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), &m_hvNew_buff);
    clSetKernelArg(kernel, 3, sizeof(cl_mem), &m_b_buff);
    clSetKernelArg(kernel, 4, sizeof(int), &m_nCells_x);
    clSetKernelArg(kernel, 5, sizeof(int), &m_nCells_y);
    clSetKernelArg(kernel, 6, sizeof(float), &i_scaling);
    clSetKernelArg(kernel, 7, sizeof(cl_mem), &m_h_buff);
    clSetKernelArg(kernel, 8, sizeof(cl_mem), &m_hv_buff);

    // Enqueue kernel

    clEnqueueNDRangeKernel(queue, kernel, 2, NULL, global_size, local_size, 0, NULL, NULL);
    clFinish(queue);

    // Read the output data back to the host

    clEnqueueReadBuffer(queue, m_h_buff, CL_TRUE, 0, sizeof(float) * (m_nCells_x + 2) * (m_nCells_y + 2), m_h, 0, NULL, NULL);
    clEnqueueReadBuffer(queue, m_hv_buff, CL_TRUE, 0, sizeof(float) * (m_nCells_x + 2) * (m_nCells_y + 2), m_hv, 0, NULL, NULL);

    clReleaseKernel(kernel);
    clReleaseCommandQueue(queue);
}

void tsunami_lab::patches::WavePropagation2d_kernel::setGhostOutflow()
{
    t_real *l_h = m_h;
    t_real *l_hu = m_hu;
    t_real *l_hv = m_hv;
    t_real *l_b = m_b;

    // set left boundary
    switch (m_state_boundary_left)
    {
    // open
    case 0:
        for (t_idx l_y = 0; l_y < m_nCells_y; l_y++)
        {
            t_idx l_coord_l = getCoordinates(0, l_y);
            t_idx l_coord_r = getCoordinates(1, l_y);
            l_h[l_coord_l] = l_h[l_coord_r];
            l_hu[l_coord_l] = l_hu[l_coord_r];
            l_hv[l_coord_l] = l_hv[l_coord_r];
            l_b[l_coord_l] = l_b[l_coord_r];
        }
        break;
    // closed
    case 1:
        for (t_idx l_y = 0; l_y < m_nCells_y; l_y++)
        {
            t_idx l_coord = getCoordinates(0, l_y);
            l_h[l_coord] = 0;
            l_hu[l_coord] = 0;
            l_hv[l_coord] = 0;
            l_b[l_coord] = 25;
        }
        break;

    default:
        std::cerr << "undefined state for left boundary" << std::endl;
        exit(EXIT_FAILURE);
        break;
    }

    // set right boundary
    switch (m_state_boundary_right)
    {
    // open
    case 0:
        for (t_idx l_y = 0; l_y < m_nCells_y; l_y++)
        {
            t_idx l_coord_l = getCoordinates(m_nCells_x, l_y);
            t_idx l_coord_r = getCoordinates(m_nCells_x + 1, l_y);
            l_h[l_coord_r] = l_h[l_coord_l];
            l_hu[l_coord_r] = l_hu[l_coord_l];
            l_hv[l_coord_r] = l_hv[l_coord_l];
            l_b[l_coord_r] = l_b[l_coord_l];
        }
        break;
    // closed
    case 1:
        for (t_idx l_y = 0; l_y < m_nCells_y; l_y++)
        {
            t_idx l_coord = getCoordinates(m_nCells_x + 1, l_y);
            l_h[l_coord] = 0;
            l_hu[l_coord] = 0;
            l_hv[l_coord] = 0;
            l_b[l_coord] = 25;
        }
        break;

    default:
        std::cerr << "undefined state for right boundary" << std::endl;
        exit(EXIT_FAILURE);
        break;
    }

    // set top boundary
    switch (m_state_boundary_top)
    {
    // open
    case 0:
        for (t_idx l_x = 0; l_x < m_nCells_x; l_x++)
        {
            t_idx l_coord_l = getCoordinates(l_x, 0);
            t_idx l_coord_r = getCoordinates(l_x, 1);
            l_h[l_coord_l] = l_h[l_coord_r];
            l_hu[l_coord_l] = l_hu[l_coord_r];
            l_hv[l_coord_l] = l_hv[l_coord_r];
            l_b[l_coord_l] = l_b[l_coord_r];
        }
        break;
    // closed
    case 1:
        for (t_idx l_x = 0; l_x < m_nCells_x; l_x++)
        {
            t_idx l_coord = getCoordinates(l_x, 0);
            l_h[l_coord] = 0;
            l_hu[l_coord] = 0;
            l_hv[l_coord] = 0;
            l_b[l_coord] = 25;
        }
        break;

    default:
        std::cerr << "undefined state for top boundary" << std::endl;
        exit(EXIT_FAILURE);
        break;
    }

    // set bottom boundary
    switch (m_state_boundary_bottom)
    {
    // open
    case 0:
        for (t_idx l_x = 0; l_x < m_nCells_x; l_x++)
        {
            t_idx l_coord_l = getCoordinates(l_x, m_nCells_y);
            t_idx l_coord_r = getCoordinates(l_x, m_nCells_y + 1);
            l_h[l_coord_r] = l_h[l_coord_l];
            l_hu[l_coord_r] = l_hu[l_coord_l];
            l_hv[l_coord_r] = l_hv[l_coord_l];
            l_b[l_coord_r] = l_b[l_coord_l];
        }
        break;
    // closed
    case 1:
        for (t_idx l_x = 0; l_x < m_nCells_x; l_x++)
        {
            t_idx l_coord = getCoordinates(l_x, m_nCells_y + 1);
            l_h[l_coord] = 0;
            l_hu[l_coord] = 0;
            l_hv[l_coord] = 0;
            l_b[l_coord] = 25;
        }
        break;

    default:
        std::cerr << "undefined state for bottom boundary" << std::endl;
        exit(EXIT_FAILURE);
        break;
    }
}
void tsunami_lab::patches::WavePropagation2d_kernel::setData()
{
    // set initial data
    clEnqueueWriteBuffer(queue, m_h_buff, CL_TRUE, 0, m_size, m_h, 0, NULL, NULL);
    clEnqueueWriteBuffer(queue, m_hu_buff, CL_TRUE, 0, m_size, m_hu, 0, NULL, NULL);
    clEnqueueWriteBuffer(queue, m_hv_buff, CL_TRUE, 0, m_size, m_hv, 0, NULL, NULL);
    clEnqueueWriteBuffer(queue, m_b_buff, CL_TRUE, 0, m_size, m_b, 0, NULL, NULL);

    clEnqueueCopyBuffer(queue, m_h_buff, m_hNew_buff, 0, 0, m_size, 0, NULL, NULL);
    clEnqueueCopyBuffer(queue, m_hu_buff, m_huNew_buff, 0, 0, m_size, 0, NULL, NULL);
    clEnqueueCopyBuffer(queue, m_hv_buff, m_hvNew_buff, 0, 0, m_size, 0, NULL, NULL);
}
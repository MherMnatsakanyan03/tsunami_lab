/**
 * @author Maurice Herold (maurice.herold AT uni-jena.de)
 * @author Mher Mnatsakanyan (mher.mnatsakanyan AT uni-jena.de)
 *
 * @section DESCRIPTION
 * Two-dimensional wave propagation patch.
 **/

#define CL_TARGET_OPENCL_VERSION 300
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
// #define PROGRAM_FILE "/Users/ibyton/Desktop/Uni/tsunami_lab/build/src/patches/wavepropagation2d_kernel/kernel.cl"
#define PROGRAM_FILE "/home/mnatsakanyan/Uni/TsunamiLab/tsunami_lab/src/patches/wavepropagation2d_kernel/kernel.cl"
// #define PROGRAM_FILE "kernel.cl"
#define KERNEL_X_AXIS_FUNC "updateXAxisKernel"
#define KERNEL_Y_AXIS_FUNC "updateYAxisKernel"
#define KERNEL_UPDATE_CELLS "mainKernel"

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

    auto device_name = std::string(256, '\0');
    clGetDeviceInfo(dev, CL_DEVICE_NAME, device_name.size(), &device_name[0], NULL);
    std::cout << "Device: " << device_name << std::endl;

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
    m_hTemp = new t_real[(m_nCells_x + 2) * (m_nCells_y + 2)]{0};
    m_hvTemp = new t_real[(m_nCells_x + 2) * (m_nCells_y + 2)]{0};

    m_b = new t_real[(m_nCells_x + 2) * (m_nCells_y + 2)]{0};

    device = create_device();

    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);

    program = build_program(context, device, PROGRAM_FILE);

    kernel = clCreateKernel(program, KERNEL_UPDATE_CELLS, &err);

    queue = clCreateCommandQueue(context, device, 0, &err);
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
    clReleaseKernel(kernel);
    clReleaseCommandQueue(queue);
}

void tsunami_lab::patches::WavePropagation2d_kernel::timeStep(t_real i_scaling)
{

    // Set kernel arguments
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &m_h_buff);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &m_hu_buff);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), &m_hv_buff);
    clSetKernelArg(kernel, 3, sizeof(cl_mem), &m_b_buff);
    clSetKernelArg(kernel, 4, sizeof(size_t), &m_nCells_x);
    clSetKernelArg(kernel, 5, sizeof(size_t), &m_nCells_y);
    clSetKernelArg(kernel, 6, sizeof(float), &i_scaling);
    clSetKernelArg(kernel, 7, sizeof(int), &m_state_boundary_left);
    clSetKernelArg(kernel, 8, sizeof(int), &m_state_boundary_right);
    clSetKernelArg(kernel, 9, sizeof(int), &m_state_boundary_top);
    clSetKernelArg(kernel, 10, sizeof(int), &m_state_boundary_bottom);
    clSetKernelArg(kernel, 11, sizeof(cl_mem), &m_hTemp_buff);
    clSetKernelArg(kernel, 12, sizeof(cl_mem), &m_huTemp_buff);
    clSetKernelArg(kernel, 13, sizeof(cl_mem), &m_hvTemp_buff);

    // size_t maxLocalSize = findMaxLocalSize(device);

    // Enqueue kernel
    size_t global_size[2] = {m_nCells_x + 2, m_nCells_y + 2}; // Gesamtanzahl der Work-Items in X- und Y-Richtung
    size_t local_size[2] = {1, 1};

    clEnqueueNDRangeKernel(queue, kernel, 2, NULL, global_size, local_size, 0, NULL, NULL);
    clFinish(queue);
}

void tsunami_lab::patches::WavePropagation2d_kernel::setData()
{
    // set initial data
    m_h_buff = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(float) * (m_nCells_x + 2) * (m_nCells_y + 2), m_h, &err);
    m_hu_buff = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(float) * (m_nCells_x + 2) * (m_nCells_y + 2), m_hu, &err);
    m_hv_buff = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(float) * (m_nCells_x + 2) * (m_nCells_y + 2), m_hv, &err);
    m_b_buff = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(float) * (m_nCells_x + 2) * (m_nCells_y + 2), m_b, &err);
    m_hTemp_buff = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(float) * (m_nCells_x + 2) * (m_nCells_y + 2), NULL, &err);
    m_huTemp_buff = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(float) * (m_nCells_x + 2) * (m_nCells_y + 2), NULL, &err);
    m_hvTemp_buff = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(float) * (m_nCells_x + 2) * (m_nCells_y + 2), NULL, &err);
}

void tsunami_lab::patches::WavePropagation2d_kernel::getData()
{
    clFinish(queue);
    clEnqueueReadBuffer(queue, m_h_buff, CL_TRUE, 0, sizeof(float) * (m_nCells_x + 2) * (m_nCells_y + 2), m_h, 0, NULL, NULL);
    clEnqueueReadBuffer(queue, m_hv_buff, CL_TRUE, 0, sizeof(float) * (m_nCells_x + 2) * (m_nCells_y + 2), m_hv, 0, NULL, NULL);
    clEnqueueReadBuffer(queue, m_hu_buff, CL_TRUE, 0, sizeof(float) * (m_nCells_x + 2) * (m_nCells_y + 2), m_hu, 0, NULL, NULL);
}

void tsunami_lab::patches::WavePropagation2d_kernel::setGhostOutflow(){};
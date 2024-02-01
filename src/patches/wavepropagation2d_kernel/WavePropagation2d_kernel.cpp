/**
 * @author Maurice Herold (maurice.herold AT uni-jena.de)
 * @author Mher Mnatsakanyan (mher.mnatsakanyan AT uni-jena.de)
 *
 * @section DESCRIPTION
 * Two-dimensional wave propagation patch using OpenCL.
 **/

#define CL_TARGET_OPENCL_VERSION 300
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS

#define KERNEL_X_AXIS_FUNC "updateXAxisKernel"
#define KERNEL_Y_AXIS_FUNC "updateYAxisKernel"
#define KERNEL_GHOSTCELLS "setGhostOutflow"
#define KERNEL_COPY "copy"

#include "WavePropagation2d_kernel.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <filesystem>
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
    m_b = new t_real[(m_nCells_x + 2) * (m_nCells_y + 2)]{0};

    device = create_device();

    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);

    std::filesystem::path currentPath = std::filesystem::current_path();
    std::string kernel_path = currentPath.string() + "/src/patches/wavepropagation2d_kernel/kernel.cl";
    const char *kernel_path_char = kernel_path.c_str();

    std::cout << "Kernel path: " << kernel_path_char << std::endl;

    program = build_program(context, device, kernel_path_char);
    ksetGhostOutflow = clCreateKernel(program, KERNEL_GHOSTCELLS, &err);
    kcopy = clCreateKernel(program, KERNEL_COPY, &err);
    knetUpdatesX = clCreateKernel(program, KERNEL_X_AXIS_FUNC, &err);
    knetUpdatesY = clCreateKernel(program, KERNEL_Y_AXIS_FUNC, &err);

    queue = clCreateCommandQueue(context, device, 0, &err);

    global_size[0] = m_nCells_x + 2; // Gesamtanzahl der Work-Items in X-Richtung
    global_size[1] = m_nCells_y + 2; // Gesamtanzahl der Work-Items in Y-Richtung
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
    clReleaseMemObject(m_hTemp_buff);
    clReleaseMemObject(m_huvTemp_buff);
    clReleaseKernel(ksetGhostOutflow);
    clReleaseKernel(kcopy);
    clReleaseKernel(knetUpdatesX);
    clReleaseKernel(knetUpdatesY);
    clReleaseCommandQueue(queue);
}

void tsunami_lab::patches::WavePropagation2d_kernel::timeStep(t_real i_scaling)
{
    // set ghost cells
    clSetKernelArg(ksetGhostOutflow, 0, sizeof(cl_mem), &m_h_buff);
    clSetKernelArg(ksetGhostOutflow, 1, sizeof(cl_mem), &m_hu_buff);
    clSetKernelArg(ksetGhostOutflow, 2, sizeof(cl_mem), &m_hv_buff);
    clSetKernelArg(ksetGhostOutflow, 3, sizeof(cl_mem), &m_b_buff);
    clSetKernelArg(ksetGhostOutflow, 4, sizeof(size_t), &m_nCells_x);
    clSetKernelArg(ksetGhostOutflow, 5, sizeof(size_t), &m_nCells_y);
    clSetKernelArg(ksetGhostOutflow, 7, sizeof(int), &m_state_boundary_left);
    clSetKernelArg(ksetGhostOutflow, 8, sizeof(int), &m_state_boundary_right);
    clSetKernelArg(ksetGhostOutflow, 9, sizeof(int), &m_state_boundary_top);
    clSetKernelArg(ksetGhostOutflow, 10, sizeof(int), &m_state_boundary_bottom);

    clEnqueueNDRangeKernel(queue, ksetGhostOutflow, 2, NULL, global_size, NULL, 0, NULL, NULL);
    clFinish(queue);

    // copy data
    clSetKernelArg(kcopy, 0, sizeof(cl_mem), &m_h_buff);
    clSetKernelArg(kcopy, 1, sizeof(cl_mem), &m_hu_buff);
    clSetKernelArg(kcopy, 2, sizeof(size_t), &m_nCells_x);
    clSetKernelArg(kcopy, 3, sizeof(size_t), &m_nCells_y);
    clSetKernelArg(kcopy, 4, sizeof(cl_mem), &m_hTemp_buff);
    clSetKernelArg(kcopy, 5, sizeof(cl_mem), &m_huvTemp_buff);

    clEnqueueNDRangeKernel(queue, kcopy, 2, NULL, global_size, NULL, 0, NULL, NULL);
    clFinish(queue);

    // update x-axis
    clSetKernelArg(knetUpdatesX, 0, sizeof(cl_mem), &m_hTemp_buff);
    clSetKernelArg(knetUpdatesX, 1, sizeof(cl_mem), &m_huvTemp_buff);
    clSetKernelArg(knetUpdatesX, 2, sizeof(cl_mem), &m_b_buff);
    clSetKernelArg(knetUpdatesX, 3, sizeof(size_t), &m_nCells_x);
    clSetKernelArg(knetUpdatesX, 4, sizeof(size_t), &m_nCells_y);
    clSetKernelArg(knetUpdatesX, 5, sizeof(float), &i_scaling);
    clSetKernelArg(knetUpdatesX, 6, sizeof(cl_mem), &m_h_buff);
    clSetKernelArg(knetUpdatesX, 7, sizeof(cl_mem), &m_hu_buff);

    clEnqueueNDRangeKernel(queue, knetUpdatesX, 2, NULL, global_size, NULL, 0, NULL, NULL);
    clFinish(queue);

    // set ghost cells
    clSetKernelArg(ksetGhostOutflow, 0, sizeof(cl_mem), &m_h_buff);
    clSetKernelArg(ksetGhostOutflow, 1, sizeof(cl_mem), &m_hu_buff);
    clSetKernelArg(ksetGhostOutflow, 2, sizeof(cl_mem), &m_hv_buff);
    clSetKernelArg(ksetGhostOutflow, 3, sizeof(cl_mem), &m_b_buff);
    clSetKernelArg(ksetGhostOutflow, 4, sizeof(size_t), &m_nCells_x);
    clSetKernelArg(ksetGhostOutflow, 5, sizeof(size_t), &m_nCells_y);
    clSetKernelArg(ksetGhostOutflow, 7, sizeof(int), &m_state_boundary_left);
    clSetKernelArg(ksetGhostOutflow, 8, sizeof(int), &m_state_boundary_right);
    clSetKernelArg(ksetGhostOutflow, 9, sizeof(int), &m_state_boundary_top);
    clSetKernelArg(ksetGhostOutflow, 10, sizeof(int), &m_state_boundary_bottom);

    clEnqueueNDRangeKernel(queue, ksetGhostOutflow, 2, NULL, global_size, NULL, 0, NULL, NULL);
    clFinish(queue);

    // copy data
    clSetKernelArg(kcopy, 0, sizeof(cl_mem), &m_h_buff);
    clSetKernelArg(kcopy, 1, sizeof(cl_mem), &m_hv_buff);
    clSetKernelArg(kcopy, 2, sizeof(size_t), &m_nCells_x);
    clSetKernelArg(kcopy, 3, sizeof(size_t), &m_nCells_y);
    clSetKernelArg(kcopy, 4, sizeof(cl_mem), &m_hTemp_buff);
    clSetKernelArg(kcopy, 5, sizeof(cl_mem), &m_huvTemp_buff);

    clEnqueueNDRangeKernel(queue, kcopy, 2, NULL, global_size, NULL, 0, NULL, NULL);
    clFinish(queue);

    // update y-axis
    clSetKernelArg(knetUpdatesY, 0, sizeof(cl_mem), &m_hTemp_buff);
    clSetKernelArg(knetUpdatesY, 1, sizeof(cl_mem), &m_huvTemp_buff);
    clSetKernelArg(knetUpdatesY, 2, sizeof(cl_mem), &m_b_buff);
    clSetKernelArg(knetUpdatesY, 3, sizeof(size_t), &m_nCells_x);
    clSetKernelArg(knetUpdatesY, 4, sizeof(size_t), &m_nCells_y);
    clSetKernelArg(knetUpdatesY, 5, sizeof(float), &i_scaling);
    clSetKernelArg(knetUpdatesY, 6, sizeof(cl_mem), &m_h_buff);
    clSetKernelArg(knetUpdatesY, 7, sizeof(cl_mem), &m_hv_buff);

    clEnqueueNDRangeKernel(queue, knetUpdatesY, 2, NULL, global_size, NULL, 0, NULL, NULL);
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
    m_huvTemp_buff = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(float) * (m_nCells_x + 2) * (m_nCells_y + 2), NULL, &err);
}

void tsunami_lab::patches::WavePropagation2d_kernel::getData()
{
    clFinish(queue);
    clEnqueueReadBuffer(queue, m_h_buff, CL_TRUE, 0, sizeof(float) * (m_nCells_x + 2) * (m_nCells_y + 2), m_h, 0, NULL, NULL);
    clEnqueueReadBuffer(queue, m_hv_buff, CL_TRUE, 0, sizeof(float) * (m_nCells_x + 2) * (m_nCells_y + 2), m_hv, 0, NULL, NULL);
    clEnqueueReadBuffer(queue, m_hu_buff, CL_TRUE, 0, sizeof(float) * (m_nCells_x + 2) * (m_nCells_y + 2), m_hu, 0, NULL, NULL);
    clFinish(queue);
}

void tsunami_lab::patches::WavePropagation2d_kernel::setGhostOutflow(){};
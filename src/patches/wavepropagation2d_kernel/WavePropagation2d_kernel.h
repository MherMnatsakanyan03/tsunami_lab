/**
 * @author Maurice Herold (maurice.herold AT uni-jena.de)
 * @author Mher Mnatsakanyan (mher.mnatsakanyan AT uni-jena.de)
 *
 * @section DESCRIPTION
 * Two-dimensional wave propagation patch.
 **/
#ifndef TSUNAMI_LAB_PATCHES_WAVE_PROPAGATION_2D_KERNEL
#define TSUNAMI_LAB_PATCHES_WAVE_PROPAGATION_2D_KERNEL

#define CL_TARGET_OPENCL_VERSION 300
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#include <string>
#include <vector>
#include "../../plugins/OpenCL/common/inc/CL/cl.h"
// #include <CL/cl.h>

#include "../WavePropagation.h"

namespace tsunami_lab
{
    namespace patches
    {
        class WavePropagation2d_kernel;
    }
} // namespace tsunami_lab

class tsunami_lab::patches::WavePropagation2d_kernel : public WavePropagation
{
private:
    //! current step which indicates the active values in the arrays below
    unsigned short m_step = 0;

    //! number of cells in x-direction discretizing the computational domain
    t_idx m_nCells_x = 0;

    //! number of cells in y-direction discretizing the computational domain
    t_idx m_nCells_y = 0;

    //! state of left boundary, 0 = open, 1 = closed
    int m_state_boundary_left = 0;

    //! state of right boundary, 0 = open, 1 = closed
    int m_state_boundary_right = 0;

    //! state of top boundary, 0 = open, 1 = closed
    int m_state_boundary_top = 0;

    //! state of bottom boundary, 0 = open, 1 = closed
    int m_state_boundary_bottom = 0;

    //! water heights for the current and next time step for all cells
    t_real *m_h = nullptr;
    //! momenta for the current and next time step for all cells in x-direction
    t_real *m_hu = nullptr;
    //! momenta for the current and next time step for all cells in y-direction
    t_real *m_hv = nullptr;

    //! bathymetry for all cells
    t_real *m_b = nullptr;

    cl_device_id device;
    cl_context context;
    cl_program program;
    cl_kernel ksetGhostOutflow;
    cl_kernel kcopy;
    cl_kernel knetUpdatesX;
    cl_kernel knetUpdatesY;
    cl_command_queue queue;
    cl_int i, err;
    size_t local_size, global_size;

    cl_mem m_b_buff;
    cl_mem m_h_buff;
    cl_mem m_hu_buff;
    cl_mem m_hv_buff;
    cl_mem m_hTemp_buff;
    cl_mem m_huvTemp_buff;

    t_idx m_size = sizeof(float) * (m_nCells_x + 2) * (m_nCells_y + 2);

    /**
     * @brief Get the 2d Coordinates of the 1d array (x-y grid is being made flat into one line)
     *
     * @param i_x x-coordinate
     * @param i_y y-coordinate
     * @return t_idx index in 1d-array
     */
    t_idx getCoordinates(t_idx i_x, t_idx i_y)
    {
        // when trying to move on a flattend 2d plane, for each y-coordinate
        // we need to "jump" one x-axis worth of a distance in the 1d-array
        return i_x + i_y * getStride();
    };

public:
    /**
     *
     * Constructs the 1d wave propagation solver.
     *
     * @param i_nCells_x number of cells in x-direction.
     * @param i_nCells_y number of cells in y-direction.
     * @param state_boundary_left type int, defines the state of the left boundary. Possible values: 0 = "open" and 1 = "closed".
     * @param state_boundary_right type int, defines the state of the right boundary. Possible values: 0 = "open" and 1 = "closed".
     * @param state_boundary_top type int, defines the state of the top boundary. Possible values: 0 = "open" and 1 = "closed".
     * @param state_boundary_bottom type int, defines the state of the bottom boundary. Possible values: 0 = "open" and 1 = "closed".
     **/
    WavePropagation2d_kernel(t_idx i_nCells_x,
                             t_idx i_nCells_y,
                             int state_boundary_left,
                             int state_boundary_right,
                             int state_boundary_top,
                             int state_boundary_bottom);

    /**
     * Destructor which frees all allocated memory.
     **/
    ~WavePropagation2d_kernel();

    /**
     * Performs a time step.
     *
     * @param i_scaling scaling of the time step (dt / dx).
     **/
    void timeStep(t_real i_scaling);

    /**
     * Sets the values of the ghost cells according to outflow boundary conditions.
     **/
    void setGhostOutflow();

    void setData();

    /**
     * Gets the stride in y-direction. x-direction is stride-1.
     *
     * @return stride in y-direction.
     **/
    t_idx getStride()
    {
        return m_nCells_x + 2;
    }

    /**
     * Gets cells' water heights.
     *
     * @return water heights.
     */
    t_real const *getHeight()
    {
        return m_h;
    }

    /**
     * Gets the cells' momenta in x-direction.
     *
     * @return momenta in x-direction.
     **/
    t_real const *getMomentumX()
    {
        return m_hu;
    }

    /**
     * Gets the cells' momenta in y-direction.
     *
     * @return momenta in y-direction.
     **/
    t_real const *getMomentumY()
    {
        return m_hv;
    }

    /**
     * Gets the cells' bathymetry.
     *
     * @return bathymetry.
     **/
    t_real const *getBathymetry()
    {
        return m_b;
    }

    /**
     * Sets the height of the cell to the given value.
     *
     * @param i_ix id of the cell in x-direction.
     * @param i_iy id of the cell in y-direction.
     * @param i_h water height.
     **/
    void setHeight(t_idx i_ix,
                   t_idx i_iy,
                   t_real i_h)
    {
        m_h[getCoordinates(i_ix + 1, i_iy + 1)] = i_h;
    }

    /**
     * Sets the momentum in x-direction to the given value.
     *
     * @param i_ix id of the cell in x-direction.
     * @param i_iy id of the cell in y-direction.
     * @param i_hu momentum in x-direction.
     **/
    void setMomentumX(t_idx i_ix,
                      t_idx i_iy,
                      t_real i_hu)
    {
        m_hu[getCoordinates(i_ix + 1, i_iy + 1)] = i_hu;
    }

    /**
     * Sets the momentum in y-direction to the given value.
     *
     * @param i_ix id of the cell in x-direction.
     * @param i_iy id of the cell in y-direction.
     * @param i_hu momentum in y-direction.
     **/
    void setMomentumY(t_idx i_ix,
                      t_idx i_iy,
                      t_real i_hv)
    {
        m_hv[getCoordinates(i_ix + 1, i_iy + 1)] = i_hv;
    }

    /**
     * @brief Set the bathymetry
     *
     * @param i_ix id of the cell in x-direction.
     * @param i_iy id of the cell in y-direction.
     * @param i_b bathymetry
     */
    void setBathymetry(t_idx i_ix,
                       t_idx i_iy,
                       t_real i_b)
    {
        m_b[getCoordinates(i_ix + 1, i_iy + 1)] = i_b;
    }

    void getData();
};

#endif
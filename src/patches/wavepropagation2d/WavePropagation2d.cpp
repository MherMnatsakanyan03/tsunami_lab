/**
 * @author Maurice Herold (maurice.herold AT uni-jena.de)
 * @author Mher Mnatsakanyan (mher.mnatsakanyan AT uni-jena.de)
 *
 * @section DESCRIPTION
 * Two-dimensional wave propagation patch.
 **/
#include "WavePropagation2d.h"

#include <iostream>
#include <stdexcept>
#include <string>

#include "../../solvers/f-wave/F_wave.h"

tsunami_lab::patches::WavePropagation2d::WavePropagation2d(t_idx i_nCells_x,
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
    for (int l_st = 0; l_st < 2; l_st++)
    {
        m_h[l_st] = new t_real[(m_nCells_x + 2) * (m_nCells_y + 2)]{0};
        m_hu[l_st] = new t_real[(m_nCells_x + 2) * (m_nCells_y + 2)]{0};
        m_hv[l_st] = new t_real[(m_nCells_x + 2) * (m_nCells_y + 2)]{0};
    }
    m_b = new t_real[(m_nCells_x + 2) * (m_nCells_y + 2)]{0};
}

tsunami_lab::patches::WavePropagation2d::~WavePropagation2d()
{
    for (int l_st = 0; l_st < 2; l_st++)
    {
        delete[] m_h[l_st];
        delete[] m_hu[l_st];
        delete[] m_hv[l_st];
    }
    delete[] m_b;
}

void tsunami_lab::patches::WavePropagation2d::timeStep(t_real i_scaling)
{
    //
    // X-AXIS
    //
    setGhostOutflow();
    // pointers to old and new data
    t_real *l_hOld = m_h[m_step];
    t_real *l_huOld = m_hu[m_step];
    t_real *l_hvOld = m_hv[m_step];

    m_step = (m_step + 1) % 2;
    t_real *l_hNew = m_h[m_step];
    t_real *l_huNew = m_hu[m_step];
    t_real *l_hvNew = m_hv[m_step];

    t_real *l_b = m_b;

// init new cell quantities
#pragma omp parallel for schedule(guided)
    for (t_idx l_y = 1; l_y < m_nCells_y + 1; l_y++)
    {
        for (t_idx l_x = 1; l_x < m_nCells_x + 1; l_x++)
        {
            t_idx l_cord = getCoordinates(l_x, l_y);
            l_hNew[l_cord] = l_hOld[l_cord];
            l_huNew[l_cord] = l_huOld[l_cord];
            l_hvNew[l_cord] = l_hvOld[l_cord];
        }
    }
    t_real l_netUpdates[2][2];

// iterate over edges and update with Riemann solutions in x-direction
#pragma omp parallel for schedule(guided)
    for (t_idx l_y = 0; l_y < m_nCells_y + 1; l_y++)
    {
        for (t_idx l_x = 0; l_x < m_nCells_x + 1; l_x++)
        {
            // determine left and right cell-id
            t_idx l_cord_L = getCoordinates(l_x, l_y);
            t_idx l_cord_R = getCoordinates(l_x + 1, l_y);

            // compute net-updates

            solvers::FWave::netUpdates(l_hOld[l_cord_L],
                                       l_hOld[l_cord_R],
                                       l_huOld[l_cord_L],
                                       l_huOld[l_cord_R],
                                       l_b[l_cord_L],
                                       l_b[l_cord_R],
                                       l_netUpdates[0],
                                       l_netUpdates[1]);

            // update the cells' quantities
            l_hNew[l_cord_L] -= i_scaling * l_netUpdates[0][0];
            l_huNew[l_cord_L] -= i_scaling * l_netUpdates[0][1];

            l_hNew[l_cord_R] -= i_scaling * l_netUpdates[1][0];
            l_huNew[l_cord_R] -= i_scaling * l_netUpdates[1][1];
        }
    }

    //
    // Y-AXIS
    //
    setGhostOutflow();
    // pointers to old and new data
    l_hOld = m_h[m_step];
    l_huOld = m_hu[m_step];
    l_hvOld = m_hv[m_step];

    m_step = (m_step + 1) % 2;
    l_hNew = m_h[m_step];
    l_huNew = m_hu[m_step];
    l_hvNew = m_hv[m_step];

    l_b = m_b;

// init new cell quantities
#pragma omp parallel for schedule(guided)
    for (t_idx l_y = 1; l_y < m_nCells_y + 1; l_y++)
    {
        for (t_idx l_x = 1; l_x < m_nCells_x + 1; l_x++)
        {
            t_idx l_cord = getCoordinates(l_x, l_y);
            l_hNew[l_cord] = l_hOld[l_cord];
            l_huNew[l_cord] = l_huOld[l_cord];
            l_hvNew[l_cord] = l_hvOld[l_cord];
        }
    }
// iterate over edges and update with Riemann solutions in y-direction
#pragma omp parallel for schedule(guided)
    for (t_idx l_y = 0; l_y < m_nCells_y + 1; l_y++)
    {
        for (t_idx l_x = 0; l_x < m_nCells_x + 1; l_x++)
        {
            // determine left and right cell-id
            t_idx l_cord_down = getCoordinates(l_x, l_y);
            t_idx l_cord_up = getCoordinates(l_x, l_y + 1);

            // compute net-updates
            t_real l_netUpdates[2][2];

            solvers::FWave::netUpdates(l_hOld[l_cord_down],
                                       l_hOld[l_cord_up],
                                       l_hvOld[l_cord_down],
                                       l_hvOld[l_cord_up],
                                       l_b[l_cord_down],
                                       l_b[l_cord_up],
                                       l_netUpdates[0],
                                       l_netUpdates[1]);

            // update the cells' quantities
            l_hNew[l_cord_down] -= i_scaling * l_netUpdates[0][0];
            l_hvNew[l_cord_down] -= i_scaling * l_netUpdates[0][1];

            l_hNew[l_cord_up] -= i_scaling * l_netUpdates[1][0];
            l_hvNew[l_cord_up] -= i_scaling * l_netUpdates[1][1];
        }
    }
}

void tsunami_lab::patches::WavePropagation2d::setGhostOutflow()
{
    t_real *l_h = m_h[m_step];
    t_real *l_hu = m_hu[m_step];
    t_real *l_hv = m_hv[m_step];
    t_real *l_b = m_b;

    // set left boundary
    switch (m_state_boundary_left)
    {
    // open
    case 0:
        for (t_idx l_y = 0; l_y < m_nCells_y; l_y++)
        {
            t_idx l_cord_l = getCoordinates(0, l_y);
            t_idx l_cord_r = getCoordinates(1, l_y);
            l_h[l_cord_l] = l_h[l_cord_r];
            l_hu[l_cord_l] = l_hu[l_cord_r];
            l_hv[l_cord_l] = l_hv[l_cord_r];
            l_b[l_cord_l] = l_b[l_cord_r];
        }
        break;
    // closed
    case 1:
        for (t_idx l_y = 0; l_y < m_nCells_y; l_y++)
        {
            t_idx l_cord = getCoordinates(0, l_y);
            l_h[l_cord] = 0;
            l_hu[l_cord] = 0;
            l_hv[l_cord] = 0;
            l_b[l_cord] = 25;
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
            t_idx l_cord_l = getCoordinates(m_nCells_x, l_y);
            t_idx l_cord_r = getCoordinates(m_nCells_x + 1, l_y);
            l_h[l_cord_r] = l_h[l_cord_l];
            l_hu[l_cord_r] = l_hu[l_cord_l];
            l_hv[l_cord_r] = l_hv[l_cord_l];
            l_b[l_cord_r] = l_b[l_cord_l];
        }
        break;
    // closed
    case 1:
        for (t_idx l_y = 0; l_y < m_nCells_y; l_y++)
        {
            t_idx l_cord = getCoordinates(m_nCells_x + 1, l_y);
            l_h[l_cord] = 0;
            l_hu[l_cord] = 0;
            l_hv[l_cord] = 0;
            l_b[l_cord] = 25;
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
            t_idx l_cord_l = getCoordinates(l_x, 0);
            t_idx l_cord_r = getCoordinates(l_x, 1);
            l_h[l_cord_l] = l_h[l_cord_r];
            l_hu[l_cord_l] = l_hu[l_cord_r];
            l_hv[l_cord_l] = l_hv[l_cord_r];
            l_b[l_cord_l] = l_b[l_cord_r];
        }
        break;
    // closed
    case 1:
        for (t_idx l_x = 0; l_x < m_nCells_x; l_x++)
        {
            t_idx l_cord = getCoordinates(l_x, 0);
            l_h[l_cord] = 0;
            l_hu[l_cord] = 0;
            l_hv[l_cord] = 0;
            l_b[l_cord] = 25;
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
            t_idx l_cord_l = getCoordinates(l_x, m_nCells_y);
            t_idx l_cord_r = getCoordinates(l_x, m_nCells_y + 1);
            l_h[l_cord_r] = l_h[l_cord_l];
            l_hu[l_cord_r] = l_hu[l_cord_l];
            l_hv[l_cord_r] = l_hv[l_cord_l];
            l_b[l_cord_r] = l_b[l_cord_l];
        }
        break;
    // closed
    case 1:
        for (t_idx l_x = 0; l_x < m_nCells_x; l_x++)
        {
            t_idx l_cord = getCoordinates(l_x, m_nCells_y + 1);
            l_h[l_cord] = 0;
            l_hu[l_cord] = 0;
            l_hv[l_cord] = 0;
            l_b[l_cord] = 25;
        }
        break;

    default:
        std::cerr << "undefined state for bottom boundary" << std::endl;
        exit(EXIT_FAILURE);
        break;
    }
}
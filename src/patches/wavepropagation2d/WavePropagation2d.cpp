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
#include <algorithm>

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
    m_h = new t_real[(m_nCells_x + 2) * (m_nCells_y + 2)]{0};
    m_hu = new t_real[(m_nCells_x + 2) * (m_nCells_y + 2)]{0};
    m_hv = new t_real[(m_nCells_x + 2) * (m_nCells_y + 2)]{0};
    m_b = new t_real[(m_nCells_x + 2) * (m_nCells_y + 2)]{0};
    m_hTemp = new t_real[(m_nCells_x + 2) * (m_nCells_y + 2)]{0};
    m_huTemp = new t_real[(m_nCells_x + 2) * (m_nCells_y + 2)]{0};
    m_hvTemp = new t_real[(m_nCells_x + 2) * (m_nCells_y + 2)]{0};
}

tsunami_lab::patches::WavePropagation2d::~WavePropagation2d()
{

    delete[] m_h;
    delete[] m_hu;
    delete[] m_hv;
    delete[] m_b;
    delete[] m_hTemp;
    delete[] m_huTemp;
    delete[] m_hvTemp;
}

void tsunami_lab::patches::WavePropagation2d::timeStep(t_real i_scaling)
{
    //
    // X-AXIS
    //
    setGhostOutflow();
    // pointers to old and new data

    std::copy(m_h, m_h + (m_nCells_x + 2) * (m_nCells_y + 2), m_hTemp);
    std::copy(m_hu, m_hu + (m_nCells_x + 2) * (m_nCells_y + 2), m_huTemp);

// iterate over edges and update with Riemann solutions in x-direction
#pragma omp parallel for schedule(guided)
    for (t_idx l_y = 0; l_y < m_nCells_y + 1; l_y++)
    {
        for (t_idx l_x = 0; l_x < m_nCells_x + 1; l_x++)
        {
            // determine left and right cell-id
            t_idx l_coord_L = getCoordinates(l_x, l_y);
            t_idx l_coord_R = getCoordinates(l_x + 1, l_y);

            // compute net-updates
            t_real l_netUpdates[2][2];

            solvers::FWave::netUpdates(m_hTemp[l_coord_L],
                                       m_hTemp[l_coord_R],
                                       m_huTemp[l_coord_L],
                                       m_huTemp[l_coord_R],
                                       m_b[l_coord_L],
                                       m_b[l_coord_R],
                                       l_netUpdates[0],
                                       l_netUpdates[1]);

            // update the cells' quantities
            m_h[l_coord_L] -= i_scaling * l_netUpdates[0][0];
            m_hu[l_coord_L] -= i_scaling * l_netUpdates[0][1];

            m_h[l_coord_R] -= i_scaling * l_netUpdates[1][0];
            m_hu[l_coord_R] -= i_scaling * l_netUpdates[1][1];
        }
    }

    //
    // Y-AXIS
    //
    setGhostOutflow();

    std::copy(m_h, m_h + (m_nCells_x + 2) * (m_nCells_y + 2), m_hTemp);
    std::copy(m_hv, m_hv + (m_nCells_x + 2) * (m_nCells_y + 2), m_hvTemp);
    // pointers to old and new data

// iterate over edges and update with Riemann solutions in y-direction
#pragma omp parallel for schedule(guided)
    for (t_idx l_y = 0; l_y < m_nCells_y + 1; l_y++)
    {
        for (t_idx l_x = 0; l_x < m_nCells_x + 1; l_x++)
        {
            // determine left and right cell-id
            t_idx l_coord_down = getCoordinates(l_x, l_y);
            t_idx l_coord_up = getCoordinates(l_x, l_y + 1);

            // compute net-updates
            t_real l_netUpdates[2][2];

            solvers::FWave::netUpdates(m_hTemp[l_coord_down],
                                       m_hTemp[l_coord_up],
                                       m_hvTemp[l_coord_down],
                                       m_hvTemp[l_coord_up],
                                       m_b[l_coord_down],
                                       m_b[l_coord_up],
                                       l_netUpdates[0],
                                       l_netUpdates[1]);

            // update the cells' quantities
            m_h[l_coord_down] -= i_scaling * l_netUpdates[0][0];
            m_hv[l_coord_down] -= i_scaling * l_netUpdates[0][1];

            m_h[l_coord_up] -= i_scaling * l_netUpdates[1][0];
            m_hv[l_coord_up] -= i_scaling * l_netUpdates[1][1];
        }
    }
}

void tsunami_lab::patches::WavePropagation2d::setGhostOutflow()
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

void tsunami_lab::patches::WavePropagation2d::setData(){};

void tsunami_lab::patches::WavePropagation2d::getData(){};
/**
 * @author Alexander Breuer (alex.breuer AT uni-jena.de)
 *
 * @section DESCRIPTION
 * IO-routines for writing a snapshot as Comma Separated Values (CSV).
 **/
#include "Csv.h"
#include <sstream>
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>

void tsunami_lab::io::Csv::write(t_real i_dxy,
                                 t_idx i_nx,
                                 t_idx i_ny,
                                 t_idx i_stride,
                                 t_real const *i_h,
                                 t_real const *i_hu,
                                 t_real const *i_hv,
                                 t_real const *i_b,
                                 std::ostream &io_stream)
{
  // write the CSV header
  io_stream << "x,y";
  if (i_h != nullptr)
    io_stream << ",height";
  if (i_hu != nullptr)
    io_stream << ",momentum_x";
  if (i_hv != nullptr)
    io_stream << ",momentum_y";
  if (i_b != nullptr)
    io_stream << ",bathymetry";
  io_stream << "\n";

  // iterate over all cells
  for (t_idx l_iy = 1; l_iy < i_ny + 1; l_iy++)
  {
    for (t_idx l_ix = 1; l_ix < i_nx + 1; l_ix++)
    {
      // derive coordinates of cell center
      t_real l_posX = (l_ix - 0.5) * i_dxy - 50;
      t_real l_posY = (l_iy - 0.5) * i_dxy - 50;

      t_idx l_id = l_iy * i_stride + l_ix;

      // write data
      io_stream << l_posX << "," << l_posY;
      if (i_h != nullptr)
        io_stream << "," << i_h[l_id];
      if (i_hu != nullptr)
        io_stream << "," << i_hu[l_id];
      if (i_hv != nullptr)
        io_stream << "," << i_hv[l_id];
      if (i_b != nullptr)
        io_stream << "," << i_b[l_id];
      io_stream << "\n";
    }
  }
  io_stream << std::flush;
}

void tsunami_lab::io::Csv::read(std::string i_filename,
                                std::vector<t_real> &o_depths)
{

  std::ifstream file(i_filename);
  std::string line, cell;

  while (std::getline(file, line))
  {
    std::stringstream lineStream(line);

    // Skip the first three values in the line
    for (int i = 0; i < 3; ++i)
    {
      std::getline(lineStream, cell, ',');
    }

    // Extract and store the fourth value
    std::getline(lineStream, cell, ',');

    o_depths.push_back(std::stof(cell));
  }
}

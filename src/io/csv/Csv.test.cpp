/**
 * @author Alexander Breuer (alex.breuer AT uni-jena.de)
 *
 * @section DESCRIPTION
 * Unit tests for the CSV-interface.
 **/
#include <catch2/catch.hpp>
#include "../../constants.h"
#include <sstream>
#define private public
#include "Csv.h"
#undef public

TEST_CASE("Test the CSV-writer for 1D settings.", "[CsvWrite1d]")
{
  // define a simple example
  tsunami_lab::t_real l_h[7] = {0, 1, 2, 3, 4, 5, 6};
  tsunami_lab::t_real l_hu[7] = {6, 5, 4, 3, 2, 1, 0};
  tsunami_lab::t_real l_b[7] = {1, 2, 3, 4, 3, 2, 1};

  std::stringstream l_stream0;
  tsunami_lab::io::Csv::write(0.5,
                              5,
                              1,
                              0,
                              0,
                              7,
                              l_h,
                              l_hu,
                              nullptr,
                              l_b,
                              l_stream0);

  std::string l_ref0 = R"V0G0N(x,y,height,momentum_x,bathymetry
0.25,0.25,1,5,2
0.75,0.25,2,4,3
1.25,0.25,3,3,4
1.75,0.25,4,2,3
2.25,0.25,5,1,2
)V0G0N";

  REQUIRE(l_stream0.str().size() == l_ref0.size());
  REQUIRE(l_stream0.str() == l_ref0);
}

TEST_CASE("Test the CSV-writer for 2D settings.", "[CsvWrite2d]")
{
  // define a simple example
  tsunami_lab::t_real l_h[16] = {0, 1, 2, 3,
                                 4, 5, 6, 7,
                                 8, 9, 10, 11,
                                 12, 13, 14, 15};
  tsunami_lab::t_real l_hu[16] = {15, 14, 13, 12,
                                  11, 10, 9, 8,
                                  7, 6, 5, 4,
                                  3, 2, 1, 0};
  tsunami_lab::t_real l_hv[16] = {0, 4, 8, 12,
                                  1, 5, 9, 13,
                                  2, 6, 10, 14,
                                  3, 7, 11, 15};
  tsunami_lab::t_real l_b[16] = {15, 14, 13, 12,
                                 11, 10, 9, 8,
                                 7, 6, 5, 4,
                                 3, 2, 1, 0};

  std::stringstream l_stream1;
  tsunami_lab::io::Csv::write(10,
                              2,
                              2,
                              0,
                              0,
                              4,
                              l_h + 4 + 1,
                              l_hu + 4 + 1,
                              l_hv + 4 + 1,
                              l_b + 4 + 1,
                              l_stream1);

  std::string l_ref1 = R"V0G0N(x,y,height,momentum_x,momentum_y,bathymetry
5,5,10,5,10,5
15,5,11,4,14,4
5,15,14,1,11,1
15,15,15,0,15,0
)V0G0N";

  REQUIRE(l_stream1.str().size() == l_ref1.size());
  REQUIRE(l_stream1.str() == l_ref1);
}

TEST_CASE("Test the CSV-reader for 1D settings.", "[CsvReader1d]")
{
  std::vector<tsunami_lab::t_real> m_b_in;
  tsunami_lab::io::Csv::read("data/real.csv", m_b_in);

  // First and last line of the exported CSV
  REQUIRE(m_b_in[0] == Approx(-8.39972685779));
  REQUIRE(m_b_in[m_b_in.size() - 1] == Approx(-5533.77099898));
}
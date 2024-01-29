/**
 * @author Alexander Breuer (alex.breuer AT uni-jena.de)
 * @author Mher Mnatsakanyan (mher.mnatsakanyan AT uni-jena.de)
 * @author Maurice Herold (maurice.herold AT uni-jena.de)
 *
 * @section DESCRIPTION
 * Entry-point for simulations.
 **/
#include <unistd.h>

#include <algorithm>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <atomic>
#include <mutex>
#include <thread>

#include "io/csv/Csv.h"
#include "io/netCDF/NetCDF.h"
#include "io/stations/Stations.h"
#include "patches/wavepropagation1d/WavePropagation1d.h"
#include "patches/wavepropagation2d/WavePropagation2d.h"
#include "patches/wavepropagation2d_kernel/WavePropagation2d_kernel.h"
#include "setups/dambreak1d/DamBreak1d.h"
#include "setups/dambreak2d/DamBreak2d.h"
#include "setups/rarerare1d/RareRare1d.h"
#include "setups/shockshock1d/ShockShock1d.h"
#include "setups/subcritical1d/Subcritical1d.h"
#include "setups/supercritical1d/Supercritical1d.h"
#include "setups/tsunamievent1d/TsunamiEvent1d.h"
#include "setups/tsunamievent2d/TsunamiEvent2d.h"
#include "setups/artificialTsunami2d/ArtificialTsunami2d.h"
#include "setups/checkpoint/Checkpoint.h"

bool do_write = true;
// declaration of variables
tsunami_lab::t_idx simulated_frame = 25;
int state_boundary_top = 0;
int state_boundary_bottom = 0;
int state_boundary_left = 0;
int state_boundary_right = 0;
tsunami_lab::t_real l_x_offset = 0;
tsunami_lab::t_real l_y_offset = 0;
int dimension;
int resolution_div = 1;
bool simulate_real_tsunami = false;
bool checkpointing = false;
bool write_parallel = true;
double checkpoint_timer = 3600.0;
int use_opencl = 0;
// std::string bat_path = "data/artificialtsunami/artificialtsunami_bathymetry_1000.nc";
// std::string dis_path = "data/artificialtsunami/artificialtsunami_displ_1000.nc";
// std::string bat_path = "data/real_tsunamis/chile_gebco20_usgs_250m_bath_fixed.nc";
// std::string dis_path = "data/real_tsunamis/chile_gebco20_usgs_250m_displ_fixed.nc";
std::string bat_path = "data/real_tsunamis/tohoku_gebco20_usgs_250m_bath.nc";
std::string dis_path = "data/real_tsunamis/tohoku_gebco20_usgs_250m_displ.nc";

void printTime(std::chrono::nanoseconds i_duration, const std::string &i_message)
{
    std::cout << i_message << ": ";

    // Berechnung und Ausgabe der Sekunden, wenn vorhanden
    if (i_duration >= std::chrono::seconds(1))
    {
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(i_duration);
        std::cout << seconds.count() << "s ";
        i_duration -= seconds;
    }

    // Berechnung und Ausgabe der Millisekunden, wenn vorhanden
    if (i_duration >= std::chrono::milliseconds(1))
    {
        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(i_duration);
        std::cout << milliseconds.count() << "ms ";
        i_duration -= milliseconds;
    }

    // Berechnung und Ausgabe der Mikrosekunden, wenn vorhanden
    if (i_duration >= std::chrono::microseconds(1))
    {
        auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(i_duration);
        std::cout << microseconds.count() << "us ";
        i_duration -= microseconds;
    }

    // Berechnung und Ausgabe der verbleibenden Nanosekunden, wenn vorhanden
    if (i_duration > std::chrono::nanoseconds(0))
    {
        std::cout << i_duration.count() << "ns ";
    }

    std::cout << std::endl;
}

int main(int i_argc,
         char *i_argv[])
{

    auto l_start_time = std::chrono::high_resolution_clock::now();
    // setenv("OMP_PROC_BIND", "master", 1);
    // setenv("OMP_PLACES", "cores", 1);

    std::filesystem::path currentPath = std::filesystem::current_path();
    std::filesystem::path targetPath;

    if (currentPath.filename() == "build")
    {
        targetPath = currentPath.parent_path() / "csv_dump";
    }
    else
    {
        targetPath = currentPath / "csv_dump";
    }

    if (!std::filesystem::exists(targetPath) && do_write)
    {
        std::filesystem::create_directory(targetPath);
    }

    // number of cells in x- and y-direction
    tsunami_lab::t_idx l_nx = 0;
    tsunami_lab::t_idx l_ny = 1;

    // set up time and print control
    tsunami_lab::t_idx l_timeStep = 0;
    tsunami_lab::t_idx l_nOut = 0;
    tsunami_lab::t_real l_simTime = 0;

    // set up filename
    std::string filename;

    // maximum observed height in the setup
    tsunami_lab::t_real l_hMax = std::numeric_limits<tsunami_lab::t_real>::lowest();

    // set cell size
    tsunami_lab::t_real l_dxy = 1;

    std::cout << "####################################" << std::endl;
    std::cout << "### Tsunami Lab                  ###" << std::endl;
    std::cout << "###                              ###" << std::endl;
    std::cout << "### https://scalable.uni-jena.de ###" << std::endl;
    std::cout << "####################################" << std::endl;

    if (std::filesystem::exists("checkpoints") && !std::filesystem::is_empty("checkpoints"))
    {
        checkpointing = true;
    }

    if (((i_argc < 4) || (i_argv[i_argc - 1][0] == '-')) && !checkpointing)
    {
        std::cerr << "invalid number of arguments OR wrong order, usage:" << std::endl;
        std::cerr << "  ./build/tsunami_lab [-d DIMENSION] [-s SETUP] [-l STATE_LEFT] [-r STATE_RIGHT] [-t STATE_TOP] [-b STATE_BOTTOM] [-i STATION] [-k RESOLUTION]  N_CELLS_X" << std::endl;
        std::cerr << "where N_CELLS_X is the number of cells in x-direction. The Grid is quadratic in 2d, so the same value will be taken for cells in y-direction" << std::endl;
        std::cerr << "The exception is 'tsunami2d', where N_CELLS_X represents the size of a cell." << std::endl;
        std::cerr << "Its is planned however to switch to a json-config based approach where everything will change." << std::endl;
        std::cerr << "-d DIMENSION = '1d','2d'" << std::endl;
        std::cerr << "When using 1d-simulation, the choices for setup are:" << std::endl;
        std::cerr << "  -s SETUP  = 'dambreak1d h_l h_r','rarerare1d h hu','shockshock1d h hu', 'supercritical1d', 'subcritical1d', 'tsunami1d'" << std::endl;
        std::cerr << "When using 2d-simulation, the choices for setup are:" << std::endl;
        std::cerr << "  -s SETUP  = 'dambreak2d', 'tsunami2d'" << std::endl;
        std::cerr << "-l STATE_LEFT = 'open','closed', default is 'open'" << std::endl;
        std::cerr << "-r STATE_RIGHT = 'open','closed', default is 'open'" << std::endl;
        std::cerr << "-t STATE_TOP = 'open','closed', default is 'open'" << std::endl;
        std::cerr << "-b STATE_BOTTOM = 'open','closed', default is 'open'" << std::endl;
        std::cerr << "-i STATION = 'path'" << std::endl;
        std::cerr << "-k RESOLUTION, where the higher the input, the lower the resolution" << std::endl;
        std::cerr << "-o OPENCL, 0 = CPU and 1 = GPU" << std::endl;
        std::cerr << "-w write parallel, 0 = parallel and 1 = normal" << std::endl;
        return EXIT_FAILURE;
    }
    else if (!checkpointing)
    {
        l_nx = atoi(i_argv[i_argc - 1]);
        if (l_nx < 1)
        {
            std::cerr << "invalid number of cells" << std::endl;
            return EXIT_FAILURE;
        }
    }

    tsunami_lab::t_real l_endTime = 1.25;
    tsunami_lab::t_real l_width = 10.0;
    std::vector<tsunami_lab::t_real> m_b_in;

    // construct solver
    tsunami_lab::patches::WavePropagation *l_waveProp;

    tsunami_lab::io::NetCdf *netcdf_manager = new tsunami_lab::io::NetCdf();

    // construct setup with default value
    tsunami_lab::setups::Setup *l_setup = new tsunami_lab::setups::DamBreak2d();
    tsunami_lab::io::Stations *l_stations = nullptr;
    l_stations = new tsunami_lab::io::Stations("data/Stations.json");

    // get command line arguments
    opterr = 0; // disable error messages of getopt
    int opt;
    if (checkpointing)
    {
        std::cout << "using checkpoint() setup" << std::endl;
        simulate_real_tsunami = true;
        dimension = 2;

        tsunami_lab::t_real l_height = -1;

        l_setup = new tsunami_lab::setups::Checkpoint();
        auto l_checkpoint = dynamic_cast<tsunami_lab::setups::Checkpoint *>(l_setup);
        l_nx = l_checkpoint->getNx();
        l_ny = l_checkpoint->getNy();
        l_x_offset = l_checkpoint->getXOffset();
        l_y_offset = l_checkpoint->getYOffset();
        state_boundary_left = l_checkpoint->getStateBoundaryLeft();
        state_boundary_right = l_checkpoint->getStateBoundaryRight();
        state_boundary_top = l_checkpoint->getStateBoundaryTop();
        state_boundary_bottom = l_checkpoint->getStateBoundaryBottom();
        l_width = l_checkpoint->getWidth();
        l_endTime = l_checkpoint->getEndTime();
        l_timeStep = l_checkpoint->getTimeStep();
        l_simTime = l_checkpoint->getTime();
        l_nOut = l_checkpoint->getNOut();
        simulated_frame = l_checkpoint->getSimulated_frame();
        l_hMax = l_checkpoint->getHMax();
        filename = l_checkpoint->getFilename();
        resolution_div = l_checkpoint->getResolutionDiv();

        l_height = l_nx * l_ny / l_width;

        std::cout << "Width: " << l_width << std::endl;
        std::cout << "Height: " << l_height << std::endl;
    }
    else
    {

        while ((opt = getopt(i_argc, i_argv, "d:s:l:r:t:b:i:k:o:w:")) != -1)
        {
            switch (opt)
            {
            case 'd':
            {
                if (std::string(optarg) == "1d")
                {
                    std::cout << "simulating in 1d" << std::endl;
                    dimension = 1;
                }
                else if (std::string(optarg) == "2d")
                {
                    std::cout << "simulating in 2d" << std::endl;
                    dimension = 2;
                }
                else
                {
                    std::cerr
                        << "undefined dimension "
                        << std::string(optarg) << std::endl
                        << "possible options are: '1d' or '2d'" << std::endl
                        << "be sure to only type in lower-case" << std::endl;
                    return EXIT_FAILURE;
                }
                break;
            }
            case 's':
            {
                std::string argument(optarg);
                std::vector<std::string> tokens;
                std::string intermediate;

                // Create a stringstream object
                std::stringstream check1(argument);

                // Tokenizing w.r.t. the delimiter ' '
                while (getline(check1, intermediate, ' '))
                {
                    tokens.push_back(intermediate);
                    std::cout << intermediate << std::endl;
                }

                delete l_setup;

                // ensure that segmentation fault is not caused
                if (((tokens[0] == "dambreak1d" || tokens[0] == "shockshock1d" || tokens[0] == "rarerare1d") && tokens.size() == 3) && dimension == 1)
                {
                    // convert to t_real
                    double l_arg1, l_arg2;
                    try
                    {
                        l_arg1 = std::stof(tokens[1]);
                        l_arg2 = std::stof(tokens[2]);
                    }
                    // if input after the name isn't a number, then throw an error
                    catch (const std::invalid_argument &ia)
                    {
                        std::cerr
                            << "Invalid argument: " << ia.what() << std::endl
                            << "be sure to only type numbers after the setup-name" << std::endl;
                        return EXIT_FAILURE;
                    }

                    if (tokens[0] == "dambreak1d")
                    {
                        std::cout << "using DamBreak1d(" << l_arg1 << ", " << l_arg2 << ", 5) setup" << std::endl;
                        l_setup = new tsunami_lab::setups::DamBreak1d(l_arg1,
                                                                      l_arg2,
                                                                      5);
                    }
                    else if (tokens[0] == "shockshock1d")
                    {
                        std::cout << "using ShockShock1d(" << l_arg1 << ", " << l_arg2 << ", 5) setup" << std::endl;
                        l_setup = new tsunami_lab::setups::ShockShock1d(l_arg1,
                                                                        l_arg2,
                                                                        5);
                    }
                    else if (tokens[0] == "rarerare1d")
                    {
                        std::cout << "using RareRare1d(" << l_arg1 << "," << l_arg2 << ", 5) setup" << std::endl;
                        l_setup = new tsunami_lab::setups::RareRare1d(l_arg1,
                                                                      l_arg2,
                                                                      5);
                    }
                    // if input isn't a defined setup, throw an error
                    else
                    {
                        std::cerr
                            << "Undefined setup: " << tokens[0] << std::endl
                            << "possible options are: 'dambreak1d', 'shockshock1d' or 'rarerare1d'" << std::endl
                            << "be sure to only type in lower-case" << std::endl;
                        return EXIT_FAILURE;
                    }
                }
                else if (tokens[0] == "subcritical1d" && dimension == 1)
                {
                    l_width = 25;
                    l_endTime = 200;
                    std::cout << "using Subcritical() setup" << std::endl;
                    l_setup = new tsunami_lab::setups::Subcritical1d();
                }
                else if (tokens[0] == "supercritical1d" && dimension == 1)
                {
                    l_width = 25;
                    l_endTime = 200;
                    std::cout << "using Supercritical() setup" << std::endl;
                    l_setup = new tsunami_lab::setups::Supercritical1d();
                }
                else if (tokens[0] == "tsunami1d" && dimension == 1)
                {
                    std::cout << "using TsunamiEvent1d() setup" << std::endl;
                    tsunami_lab::io::Csv::read("data/real.csv", m_b_in);

                    l_width = 250 * m_b_in.size();
                    l_endTime = 3600;

                    l_setup = new tsunami_lab::setups::TsunamiEvent1d(m_b_in);
                }
                else if (tokens[0] == "tsunami2d" && dimension == 2)
                {
                    std::cout << "using TsunamiEvent2d() setup" << std::endl;
                    simulate_real_tsunami = true;
                    l_endTime = 36000;

                    tsunami_lab::t_real l_height = -1;
                    l_setup = new tsunami_lab::setups::TsunamiEvent2d(bat_path,
                                                                      dis_path,
                                                                      &l_width,
                                                                      &l_height,
                                                                      &l_x_offset,
                                                                      &l_y_offset);

                    std::cout << "Width: " << l_width << std::endl;
                    std::cout << "Height: " << l_height << std::endl;
                    // in this case l_nx is initially to interpret as the cell-length in meter (l_dxy)
                    // with this, we can now get the cell count dynamically, depending on the input file
                    // (same with l_ny, its the ratio of heigth to width times the x-cell-count)
                    // credits to Justus Dreßler for giving the idea of dynamic cell-count calculation
                    l_nx = l_width / l_nx;
                    l_ny = l_nx * l_height / l_width;

                    simulated_frame = 500;
                }
                else if (tokens[0] == "artificial2d" && dimension == 2)
                {
                    std::cout << "using ArtificialTsunami2d() setup" << std::endl;

                    l_width = 10000;
                    l_x_offset = 5000;
                    l_y_offset = 5000;
                    l_endTime = 300;

                    l_setup = new tsunami_lab::setups::ArtificialTsunami2d();
                }
                else if (tokens[0] == "dambreak2d" && dimension == 2)
                {
                    l_width = 100;
                    l_endTime = 15;
                    l_x_offset = 0;
                    l_y_offset = 0;
                    std::cout << "using Dambreak2d() setup" << std::endl;
                    l_setup = new tsunami_lab::setups::DamBreak2d();
                }
                else
                {
                    // if input doesn't follow the regulations "<name> <arg1> <arg2>"
                    // OR dimension does't match with the setup
                    std::cerr
                        << "Either: False number of arguments for setup: " << tokens.size() << std::endl
                        << "Expected: 3" << std::endl
                        << "OR: Wrong dimension-setup-combination" << std::endl;
                    return EXIT_FAILURE;
                }
                break;
            }
            case 'l':
            {
                if (std::string(optarg) == "open")
                {
                    std::cout << "left-boundary open" << std::endl;
                    state_boundary_left = 0;
                }
                else if (std::string(optarg) == "closed")
                {
                    std::cout << "left-boundary closed" << std::endl;
                    state_boundary_left = 1;
                }
                else
                {
                    std::cerr
                        << "unknown state "
                        << std::string(optarg) << std::endl
                        << "possible options are: 'open' or 'closed'" << std::endl
                        << "be sure to only type in lower-case" << std::endl;
                    return EXIT_FAILURE;
                }
                break;
            }
            case 'r':
            {
                if (std::string(optarg) == "open")
                {
                    std::cout << "right-boundary open" << std::endl;
                    state_boundary_right = 0;
                }
                else if (std::string(optarg) == "closed")
                {
                    std::cout << "right-boundary closed" << std::endl;
                    state_boundary_right = 1;
                }
                else
                {
                    std::cerr
                        << "unknown state "
                        << std::string(optarg) << std::endl
                        << "possible options are: 'open' or 'closed'" << std::endl
                        << "be sure to only type in lower-case" << std::endl;
                    return EXIT_FAILURE;
                }
                break;
            }
            case 't':
            {
                if (std::string(optarg) == "open")
                {
                    std::cout << "top-boundary open" << std::endl;
                    state_boundary_top = 0;
                }
                else if (std::string(optarg) == "closed")
                {
                    std::cout << "top-boundary closed" << std::endl;
                    state_boundary_top = 1;
                }
                else
                {
                    std::cerr
                        << "unknown state "
                        << std::string(optarg) << std::endl
                        << "possible options are: 'open' or 'closed'" << std::endl
                        << "be sure to only type in lower-case" << std::endl;
                    return EXIT_FAILURE;
                }
                break;
            }
            case 'b':
            {
                if (std::string(optarg) == "open")
                {
                    std::cout << "bottom-boundary open" << std::endl;
                    state_boundary_bottom = 0;
                }
                else if (std::string(optarg) == "closed")
                {
                    std::cout << "bottom-boundary closed" << std::endl;
                    state_boundary_bottom = 1;
                }
                else
                {
                    std::cerr
                        << "unknown state "
                        << std::string(optarg) << std::endl
                        << "possible options are: 'open' or 'closed'" << std::endl
                        << "be sure to only type in lower-case" << std::endl;
                    return EXIT_FAILURE;
                }
                break;
            }
            case 'i':
            {
                std::string i_filePath(optarg);
                l_stations = new tsunami_lab::io::Stations(i_filePath);
                break;
            }
            case 'k':
            {
                resolution_div = atoi(optarg);
                if (resolution_div < 1)
                {
                    std::cout << "Error: resolution-scalar cannot be less than 1." << std::endl;
                    return EXIT_FAILURE;
                }
                break;
            }
            case 'o':
            {
                use_opencl = atoi(optarg);

                break;
            }
            case 'w':
            {
                if (std::string(optarg) == "1")
                {
                    write_parallel = true;
                    std::cout << "write parallel" << std::endl;
                }
                else if (std::string(optarg) == "0")
                {
                    write_parallel = false;
                    std::cout << "write normal" << std::endl;
                }
                else
                {
                    std::cerr
                        << "undefined write parallel "
                        << std::string(optarg) << std::endl
                        << "possible options are: '0' or '1'" << std::endl
                        << "be sure to only type in lower-case" << std::endl;
                    return EXIT_FAILURE;
                }
                break;
            }
            // unknown option
            case '?':
            {
                std::cerr
                    << "Undefinded option: " << char(optopt) << " OR wrong dimension-setup-combination" << std::endl
                    << "possible options are:" << std::endl
                    << "    -d DIMENSION = '1d','2d'" << std::endl
                    << "    When using 1d-simulation, the choices for setup are:" << std::endl
                    << "        -s SETUP  = 'dambreak h_l h_r','rarerare h hu','shockshock h hu', 'supercritical', 'subcritical', 'tsunami'" << std::endl
                    << "    When using 2d-simulation, the choices for setup are:" << std::endl
                    << "        -s SETUP  = 'dambreak', 'tsunami2d'" << std::endl
                    << "    -l STATE_LEFT = 'open','closed', default is 'open'" << std::endl
                    << "    -r STATE_RIGHT = 'open','closed', default is 'open'" << std::endl
                    << "    -t STATE_TOP = 'open','closed', default is 'open'" << std::endl
                    << "    -b STATE_BOTTOM = 'open','closed', default is 'open'" << std::endl
                    << "    -i 'path' " << std::endl
                    << "    -k RESOLUTION, where the higher the input, the lower the resolution" << std::endl
                    << "    -o OPENCL, 0 = CPU and 1 = GPU" << std::endl;
                break;
            }
            }
        }
    }

    switch (dimension)
    {
    case 1:
        if (use_opencl)
        {
            std::cout << "Using OpenCL in 1d is not supported. Exiting." << std::endl;
            return EXIT_FAILURE;
        }
        l_waveProp = new tsunami_lab::patches::WavePropagation1d(l_nx,
                                                                 state_boundary_left,
                                                                 state_boundary_right);
        break;

    case 2:
        if (!simulate_real_tsunami)
        {
            l_ny = l_nx;
        }
        if (use_opencl)
        {
            l_waveProp = new tsunami_lab::patches::WavePropagation2d_kernel(l_nx,
                                                                            l_ny,
                                                                            state_boundary_left,
                                                                            state_boundary_right,
                                                                            state_boundary_top,
                                                                            state_boundary_bottom);
        }
        else
        {
            l_waveProp = new tsunami_lab::patches::WavePropagation2d(l_nx,
                                                                     l_ny,
                                                                     state_boundary_left,
                                                                     state_boundary_right,
                                                                     state_boundary_top,
                                                                     state_boundary_bottom);
        }

        break;

    default:
        std::cerr << "Dimension has to be specified" << std::endl;
        return EXIT_FAILURE;
        break;
    }

    l_dxy = l_width / l_nx;

    std::cout << "runtime configuration" << std::endl;
    std::cout << "  number of cells in x-direction: " << l_nx << std::endl;
    std::cout << "  number of cells in y-direction: " << l_ny << std::endl;
    std::cout << "  cell size:                      " << l_dxy << std::endl;

    // set up solver
    for (tsunami_lab::t_idx l_cy = 0; l_cy < l_ny; l_cy++)
    {
        tsunami_lab::t_real l_y = l_cy * l_dxy - l_y_offset;

        for (tsunami_lab::t_idx l_cx = 0; l_cx < l_nx; l_cx++)
        {
            tsunami_lab::t_real l_x = l_cx * l_dxy - l_x_offset;

            tsunami_lab::t_real l_h, l_hu, l_hv, l_b;

            // get initial values of the setup
            if (checkpointing)
            {
                l_h = l_setup->getHeight(l_cx,
                                         l_cy);
                l_hu = l_setup->getMomentumX(l_cx,
                                             l_cy);
                l_hv = l_setup->getMomentumY(l_cx,
                                             l_cy);
                l_b = l_setup->getBathymetry(l_cx,
                                             l_cy);
            }
            else
            {
                l_h = l_setup->getHeight(l_x,
                                         l_y);
                l_hMax = std::max(l_h, l_hMax);

                l_hu = l_setup->getMomentumX(l_x,
                                             l_y);
                l_hv = l_setup->getMomentumY(l_x,
                                             l_y);
                l_b = l_setup->getBathymetry(l_x,
                                             l_y);
            }

            // set initial values in wave propagation solver
            l_waveProp->setHeight(l_cx,
                                  l_cy,
                                  l_h);

            l_waveProp->setMomentumX(l_cx,
                                     l_cy,
                                     l_hu);

            l_waveProp->setMomentumY(l_cx,
                                     l_cy,
                                     l_hv);

            l_waveProp->setBathymetry(l_cx,
                                      l_cy,
                                      l_b);
        }
    }
    l_waveProp->setData();
    if (dimension == 2 && !checkpointing && do_write)
    {
        /* if (std::filesystem::exists("netCDF_dump"))
        {
            std::filesystem::remove_all("netCDF_dump");
        } */

        // create netCDF_dump folder
        std::filesystem::create_directory("netCDF_dump");

        std::time_t t = std::time(nullptr);

        filename = "netCDF_dump/netCDFdump_" + std::to_string(l_dxy) + "_ " + std::to_string(t) + ".nc";

        netcdf_manager->initialize(filename,
                                   l_dxy,
                                   l_nx,
                                   l_ny,
                                   resolution_div,
                                   l_x_offset,
                                   l_y_offset,
                                   netcdf_manager->removeGhostCells(l_waveProp->getBathymetry(), l_nx, l_ny, 1, 1, l_waveProp->getStride()));
    }

    // derive maximum wave speed in setup; the momentum is ignored
    tsunami_lab::t_real l_speedMax = std::sqrt(9.81 * l_hMax);

    // derive constant time step; changes at simulation time are ignored
    tsunami_lab::t_real l_dt = 0.5 * l_dxy / l_speedMax;

    // derive scaling for a time step
    tsunami_lab::t_real l_scaling = l_dt / l_dxy;

    std::cout << "entering time loop" << std::endl;

    // clear csv_dump
    if (std::filesystem::exists("csv_dump"))
    {
        std::filesystem::remove_all("csv_dump");
    }

    // create csv_dump folder
    if (do_write)
    {
        std::filesystem::create_directory("csv_dump");
    }

    // clear station_data
    if (std::filesystem::exists("station_data"))
    {
        std::filesystem::remove_all("station_data");
    }

    // create station_data folder
    if (do_write)
    {
        std::filesystem::create_directory("station_data");
    }

    int multiplier = 0;
    auto l_lastCheckpointTime = std::chrono::high_resolution_clock::now();
    auto l_setup_time = std::chrono::high_resolution_clock::now();
    std::chrono::nanoseconds l_duration_write = std::chrono::nanoseconds::zero();
    std::chrono::nanoseconds l_duration_checkpoint = std::chrono::nanoseconds::zero();

    // setup for write thread
    std::mutex write_mutex;
    std::atomic<bool> is_write_completed(true);
    std::condition_variable write_condition;
    write_condition.notify_one();

    // iterate over time
    while (l_simTime < l_endTime)
    {
        auto l_currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> l_elapsedTime = l_currentTime - l_start_time;

        if (l_elapsedTime.count() >= checkpoint_timer && dimension == 2 && do_write)
        {
            netcdf_manager->writeCheckpoint(l_nx,
                                            l_ny,
                                            netcdf_manager->removeGhostCells(l_waveProp->getHeight(),
                                                                             l_nx,
                                                                             l_ny,
                                                                             1,
                                                                             1,
                                                                             l_waveProp->getStride()),
                                            netcdf_manager->removeGhostCells(l_waveProp->getMomentumX(),
                                                                             l_nx,
                                                                             l_ny,
                                                                             1,
                                                                             1,
                                                                             l_waveProp->getStride()),
                                            netcdf_manager->removeGhostCells(l_waveProp->getMomentumY(),
                                                                             l_nx,
                                                                             l_ny,
                                                                             1,
                                                                             1,
                                                                             l_waveProp->getStride()),
                                            netcdf_manager->removeGhostCells(l_waveProp->getBathymetry(),
                                                                             l_nx,
                                                                             l_ny,
                                                                             1,
                                                                             1,
                                                                             l_waveProp->getStride()),
                                            l_x_offset,
                                            l_y_offset,
                                            state_boundary_left,
                                            state_boundary_right,
                                            state_boundary_top,
                                            state_boundary_bottom,
                                            l_width,
                                            l_endTime,
                                            l_timeStep,
                                            l_simTime,
                                            l_nOut,
                                            l_hMax,
                                            simulated_frame,
                                            resolution_div,
                                            filename);
            l_lastCheckpointTime = std::chrono::high_resolution_clock::now();
            l_duration_checkpoint += l_lastCheckpointTime - l_currentTime;
        }
        if (l_timeStep % simulated_frame == 0)
        {

            // Start of local timer
            auto l_localStartTime = std::chrono::high_resolution_clock::now();

            std::cout << "  simulation time / #time steps: "
                      << l_simTime << " / " << l_timeStep << std::endl;

            if (dimension == 1 && do_write)
            {
                std::string l_path = targetPath.string() + "/" + "solution_" + std::to_string(l_nOut) + ".csv";
                std::cout << "  writing wave field to " << l_path << std::endl;

                std::ofstream l_file;
                l_file.open(l_path);
                tsunami_lab::io::Csv::write(l_dxy,
                                            l_nx,
                                            l_ny,
                                            l_x_offset,
                                            l_y_offset,
                                            l_waveProp->getStride(),
                                            l_waveProp->getHeight(),
                                            l_waveProp->getMomentumX(),
                                            l_waveProp->getMomentumY(),
                                            l_waveProp->getBathymetry(),
                                            l_file);

                l_file.close();
            }
            else if (dimension == 2 && do_write && !write_parallel)
            {
                l_waveProp->getData();
                netcdf_manager->write(l_nx,
                                      l_ny,
                                      resolution_div,
                                      netcdf_manager->removeGhostCells(l_waveProp->getHeight(),
                                                                       l_nx,
                                                                       l_ny,
                                                                       1,
                                                                       1,
                                                                       l_waveProp->getStride()),
                                      netcdf_manager->removeGhostCells(l_waveProp->getMomentumX(),
                                                                       l_nx,
                                                                       l_ny,
                                                                       1,
                                                                       1,
                                                                       l_waveProp->getStride()),
                                      netcdf_manager->removeGhostCells(l_waveProp->getMomentumY(),
                                                                       l_nx,
                                                                       l_ny,
                                                                       1,
                                                                       1,
                                                                       l_waveProp->getStride()),
                                      l_nOut,
                                      l_simTime,
                                      filename);
            }
            else if (dimension == 2 && do_write && write_parallel)
            {
                // lock until Writhe Thread is done
                std::unique_lock<std::mutex> lock(write_mutex);
                write_condition.wait(lock, [&]
                                     { return is_write_completed.load(); });

                l_waveProp->getData();
                auto n_out = l_nOut;
                auto n_simTime = l_simTime;
                lock.unlock();
                std::thread write_thread([&, n_out, n_simTime]()
                                         {
                                             std::unique_lock<std::mutex> lock(write_mutex);
                                             netcdf_manager->write(l_nx,
                                                                   l_ny,
                                                                   resolution_div,
                                                                   netcdf_manager->removeGhostCells(l_waveProp->getHeight(),
                                                                                                    l_nx,
                                                                                                    l_ny,
                                                                                                    1,
                                                                                                    1,
                                                                                                    l_waveProp->getStride()),
                                                                   netcdf_manager->removeGhostCells(l_waveProp->getMomentumX(),
                                                                                                    l_nx,
                                                                                                    l_ny,
                                                                                                    1,
                                                                                                    1,
                                                                                                    l_waveProp->getStride()),
                                                                   netcdf_manager->removeGhostCells(l_waveProp->getMomentumY(),
                                                                                                    l_nx,
                                                                                                    l_ny,
                                                                                                    1,
                                                                                                    1,
                                                                                                    l_waveProp->getStride()),
                                                                   n_out,
                                                                   n_simTime,
                                                                   filename);
                                             lock.unlock();
                                             is_write_completed = true;
                                             write_condition.notify_one(); });
                write_thread.detach();
            }
            l_nOut++;

            // End of local timer
            auto l_localEndTime = std::chrono::high_resolution_clock::now();
            std::chrono::nanoseconds l_localElapsedTime = l_localEndTime - l_localStartTime;
            l_duration_write += l_localElapsedTime;

            printTime(l_localElapsedTime, "write time");
            // Global timer
            std::cout << "\tTime since programm started: " << l_elapsedTime.count() << "s" << std::endl;
        }

        if (l_simTime >= multiplier && do_write)
        {
            l_stations->writeStationOutput(l_dxy,
                                           l_nx,
                                           l_ny,
                                           l_x_offset,
                                           l_y_offset,
                                           l_waveProp->getStride(),
                                           l_waveProp->getHeight(),
                                           l_waveProp->getMomentumX(),
                                           l_waveProp->getMomentumY(),
                                           l_waveProp->getBathymetry(),
                                           l_simTime);
            multiplier += l_stations->getOutputFrequency();
        }

        l_waveProp->timeStep(l_scaling);

        l_timeStep++;
        l_simTime += l_dt;
    }

    auto l_end = std::chrono::high_resolution_clock::now();
    auto l_duration_total = l_end - l_start_time;
    auto l_duration_setup = l_setup_time - l_start_time;
    auto l_duration_loop = l_end - l_setup_time;
    auto l_duration_calc = l_duration_loop - l_duration_write - l_duration_checkpoint;
    printTime(l_duration_total, "total time");
    printTime(l_duration_calc, "calc time ");
    printTime(l_duration_setup, "setup time");
    printTime(l_duration_write, "write time");
    printTime(l_duration_checkpoint, "checkpoint time");

    std::cout << "finished time loop" << std::endl;

    // free memory
    std::cout << "freeing memory: l_setup" << std::endl;
    delete l_setup;
    std::cout << "freeing memory: l_waveProp" << std::endl;
    delete l_waveProp;
    std::cout << "freeing memory: l_stations" << std::endl;
    delete l_stations;
    std::cout << "freeing memory: netcdf_manager" << std::endl;
    delete netcdf_manager;

    // clear checkpoint
    std::cout << "delete checkpoints" << std::endl;
    if (std::filesystem::exists("checkpoints"))
    {
        std::filesystem::remove_all("checkpoints");
    }

    std::cout << "finished, exiting" << std::endl;
    return EXIT_SUCCESS;
}

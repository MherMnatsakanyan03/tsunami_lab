9 Parallelization
=================

Links:
------------

`Github Repo <https://github.com/MherMnatsakanyan03/tsunami_lab.git>`_


Individual Contributions:
-------------------------

Mher Mnatsakanyan and Maurice Herold did a similar amount of work.

9.1: Parallelization
--------------------

9.1.1: Implementation
^^^^^^^^^^^^^^^^^^^^^

Using the OpenMP library, we had the choice of parallelizing the code in two ways. We could either parallelize
the outer loop or we could parallelize the inner loop. In task 9.1.3, we will compare the two approaches.
Spoiler: parallelizing the outer is faster, which is why the following code shows the outer loop parallelization.

.. code:: c++

    // init new cell quantities
    #pragma omp parallel for
        for (t_idx l_y = 1; l_y < m_nCells_y + 1; l_y++)
        {
            for (t_idx l_x = 1; l_x < m_nCells_x + 1; l_x++)
            {
                t_idx l_coord = getCoordinates(l_x, l_y);
                l_hNew[l_coord] = l_hOld[l_coord];
                l_huNew[l_coord] = l_huOld[l_coord];
                l_hvNew[l_coord] = l_hvOld[l_coord];
            }
        }
    // iterate over edges and update with Riemann solutions in y-direction
    #pragma omp parallel for
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

We also decided to remove the roe solver from our project entrierly, since it was outdated and for the future
plan of adding hardware-acceleration, it would be better to use as little if-conditionals as possible.


9.1.2: Ara-cluster
^^^^^^^^^^^^^^^^^^

As we will see in 9.1.3, parallelizing the outer loop is the fastest way to parallelize the code (in a simple
way). That is the reason, why in the following we will only show the results of parallelizing the outer loop.


9.1.3: Inner vs Outer Loop
^^^^^^^^^^^^^^^^^^^^^^^^^^

The following outputs show the time it took to run the program with different implementations of the
parallelization considering the placement of the pragma omp parallel for.
The tests were run on our private computer, which has an Intel(R) Core(TM) i5-14600KF CPU @ 3.70GHz with 14
cores and 20 threads. The programm was utilizing all 20 physical/virtual cores, which was confirmed by the task
manager.

.. code:: python

    # no parallelization
    simulation time / #time steps: 35878.6 / 11000
        Time to write: 3.4471e-05s
        Time since programm started: 211.845s

    # inner loop parallelization
    simulation time / #time steps: 35878.6 / 11000
        Time to write: 0.00017454s
        Time since programm started: 123.914s

    # outer loop parallelization
    simulation time / #time steps: 35878.6 / 11000
        Time to write: 0.000143608s
        Time since programm started: 26.9835s

    # both loops parallelized (using collapse(2))
    simulation time / #time steps: 35878.6 / 11000
        Time to write: 0.000239023s
        Time since programm started: 27.0409s

As we can see, any form of parallelization is faster than no parallelization at all. While we had the
concern that parallelizing the inner loop would cause an error in the simulation, we were able to confirm
that the results were the same. The resulting animation seemed fine and after thinking it through, we
were not able to find the ground of our concerns.

What is interesting to see is that parallelizing both loops is slower than parallelizing only the outer loop.
We believe that the reason for that is the lack of cores to parallelize all of the possible threads.
This would make the overhead to create the inner threads bigger than the time saved by parallelizing them
while still parallelizing the more efficient outer loop.


9.1.4: Schedule and Pinning stategies
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In the following, we will test out different schedule-tecniques provided by the OpenMP library using the outer
loop parallelization.

There are 3 different schedule-types: static, dynamic and guided. The static schedule divides the iterations
into chunks of size :code:`chunk_size` and assigns them to the threads. The dynamic schedule assigns the
iterations to the threads as they finish their work, which creates an additional overhead. The guided schedule
is similar to the dynamic schedule, but the chunk size is proportional to the number of unassigned iterations
divided by the number of the threads, decreasing over time.

.. code:: python

    #default schedule
    simulation time / #time steps: 35878.6 / 11000
        Time to write: 4.4317e-05s
        Time since programm started: 27.7263s

    # static schedule
    simulation time / #time steps: 35878.6 / 11000
        Time to write: 0.000153224s
        Time since programm started: 27.6077s

    # dynamic schedule
    simulation time / #time steps: 35878.6 / 11000
        Time to write: 0.000157308s
        Time since programm started: 29.5667s

    # guided schedule
    simulation time / #time steps: 35878.6 / 11000
        Time to write: 0.000172596s
        Time since programm started: 25.1368s

The overhead for the tsunami simulation is too big for the dynamic schedule-tecnique to be faster than the
static schedule. The guided schedule is the fastest.

The OpenMP library also provides the possibility to pin threads to cores. This means that the threads will
always run on the same core.

We found the implementation for this to be the :code:`setenv("OMP_PROC_BIND", VALUE, OVERWRITE);` environment
variable. The :code:`VALUE` can be :code:`true`, :code:`false`, :code:`spread`, :code:`close` or :code:`master`.

Using different implementations of:

.. code:: c++
    
    int main(int i_argc,
            char *i_argv[])
    {
        setenv("OMP_PROC_BIND", "true/false/master/close/spread", true);
        setenv("OMP_PLACES", "cores/threads", true);
        ...

and multiple runs of the simulation, we found that these parameters didn't have any effect on the simulation
time. Each thread is using independent data, so there is no need for the threads to communicate with each other, meaning that pinning strategies are not needed. At least that is our theory.
Command Line Interface
======================

A Command Line Interface (CLI) is also provided for running long simulations or parameter fitting:

*  |icon-linux|_ `linux <https://github.com/spatial-model-editor/spatial-model-editor/releases/latest/download/spatial-cli>`_
*  |icon-osx-arm64|_ `macOS arm64 <https://github.com/spatial-model-editor/spatial-model-editor/releases/latest/download/spatial-cli-ARM64.dmg>`_
*  |icon-osx|_ `macOS <https://github.com/spatial-model-editor/spatial-model-editor/releases/latest/download/spatial-cli.dmg>`_
*  |icon-windows|_ `windows <https://github.com/spatial-model-editor/spatial-model-editor/releases/latest/download/spatial-cli.exe>`_

.. |icon-linux| image:: ../img/icon-linux.png
.. _icon-linux: https://github.com/spatial-model-editor/spatial-model-editor/releases/latest/download/spatial-cli

.. |icon-osx-arm64| image:: ../img/icon-osx.png
.. _icon-osx-arm64: https://github.com/spatial-model-editor/spatial-model-editor/releases/latest/download/spatial-cli-ARM64.dmg

.. |icon-osx| image:: ../img/icon-osx.png
.. _icon-osx: https://github.com/spatial-model-editor/spatial-model-editor/releases/latest/download/spatial-cli.dmg

.. |icon-windows| image:: ../img/icon-windows.png
.. _icon-windows: https://github.com/spatial-model-editor/spatial-model-editor/releases/latest/download/spatial-cli.exe

Simulation
----------

It can be used to simulate a model and save the results.
For example, this would simulate the model from the file ``filename.xml`` for ten units of time, with 1 unit of time between images, and store the results in ``results.sme``:

.. code-block:: bash

    ./spatial-cli simulate filename.xml 10 1 -o results.sme

The file ``results.sme`` can be opened in the GUI to see the simulation results.

An existing simulation can also be continued, for example this would simulate 5 more steps of length 1 and append the results to the existing simulation results in ``results.sme``:

.. code-block:: bash

    ./spatial-cli simulate results.sme 5 1

If the output file is not specified it defaults to overwriting the input file.

Multiple time intervals can be specified as semicolon delimited lists, the same as in the GUI.
For example:

.. code-block:: bash

    ./spatial-cli simulate results.sme "5;25;10" "1;2.5;0.1"

Parameter fitting
-----------------

If parameter optimization has been set up in the model (this can be done in the GUI), the parameter fitting subcommand can be used to run the optimization algorithm.
For example, to run 50 iterations of the particle swarm optimization algorithm on the model in ``filename.xml``,
using 4 optimization threads with a population of 20 per thread,
using the Pixel simulator with a single thread for the simulation,
and save the model with the the optimized parameters results in ``results.sme``:

.. code-block:: bash

    ./spatial-cli fit filename.xml -s pixel -t 1 -a PSO -j 4 -p 20 -i 50 -o results.sme

Command line parameters
-----------------------

`spatial-cli --help` displays the available options and subcommands:

.. code-block:: bash

    Spatial Model Editor CLI v1.9.0


    ./cli/spatial-cli [OPTIONS] SUBCOMMAND


    OPTIONS:
      -h,     --help              Print this help message and exit
      -v,     --version           Display program version information and exit
      -d,     --dump-config       Dump the default config ini file and exit
      -c,     --config            Read an ini file containing simulation options

    SUBCOMMANDS:
      simulate                    Run a spatial simulation
      fit                         Run parameter fitting

`spatial-cli simulate --help` displays the available options for the simulate subcommand:

.. code-block:: bash

    Run a spatial simulation


    ./spatial-cli simulate [OPTIONS] file times image-intervals


    POSITIONALS:
      file TEXT:FILE REQUIRED     The spatial SBML model to simulate
      times TEXT REQUIRED         The simulation time(s) (in model units of time, separated by ';')
      image-intervals TEXT REQUIRED
                                  The interval(s) between saving images (in model units of time)

    OPTIONS:
      -h,     --help              Print this help message and exit
      -s,     --simulator ENUM:value in {dune->0,pixel->1} OR {0,1} [0]
                                  The simulator to use: dune or pixel
      -t,     --nthreads UINT:NONNEGATIVE [0]
                                  The maximum number of CPU threads to use when simulating (0 means
                                  unlimited)
      -o,     --output-file TEXT  The output file to write the results to. If not set, then the
                                  input file is used.

`spatial-cli fit --help` displays the available options for the parameter fitting subcommand:

.. code-block:: bash

    Run parameter fitting


    ./spatial-cli fit [OPTIONS] file


    POSITIONALS:
      file TEXT:FILE REQUIRED     The spatial SBML model to simulate

    OPTIONS:
      -h,     --help              Print this help message and exit
      -s,     --simulator ENUM:value in {dune->0,pixel->1} OR {0,1} [0]
                                  The simulator to use: dune or pixel
      -t,     --nthreads UINT:NONNEGATIVE [0]
                                  The maximum number of CPU threads to use when simulating (0 means
                                  unlimited)
      -o,     --output-file TEXT  The output file to write the results to. If not set, then the
                                  input file is used.
      -a,     --algorithm ENUM:value in {ABC->6,AL->12,BOBYQA->9,COBYLA->8,DE->2,GPSO->1,NMS->10,PRAXIS->13,PSO->0,gaco->7,iDE->3,jDE->4,pDE->5,sbplx->11} OR {6,12,9,8,2,1,10,13,0,7,3,4,5,11} [0]
                                  The optimization algorithm to use
      -i,     --n-iterations UINT:POSITIVE [20]
                                  The number of iterations to run the fitting algorithm
      -p,     --population-per-thread UINT:POSITIVE [20]
                                  The population per optimization thread
      -j,     --n-threads UINT:POSITIVE [1]
                                  The number of optimization threads


Using a config file
-------------------

To create an ini file with all options set to their default values, you can use the ``-d`` option:

.. code-block:: bash

    ./spatial-cli -d > config.ini

You can then edit this file as desired, and use it with the ``-c`` option instead of specifying the options on the command line:

.. code-block:: bash

    ./spatial-cli fit -c config.ini

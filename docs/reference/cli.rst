Command Line Interface
======================

A Command Line Interface (CLI) is also provided for running long simulations:

*  |icon-linux|_ `linux <https://github.com/spatial-model-editor/spatial-model-editor/releases/latest/download/spatial-cli>`_
*  |icon-osx|_ `macOS <https://github.com/spatial-model-editor/spatial-model-editor/releases/latest/download/spatial-cli.dmg>`_
*  |icon-windows|_ `windows <https://github.com/spatial-model-editor/spatial-model-editor/releases/latest/download/spatial-cli.exe>`_

.. |icon-linux| image:: ../img/icon-linux.png
.. _icon-linux: https://github.com/spatial-model-editor/spatial-model-editor/releases/latest/download/spatial-cli

.. |icon-osx| image:: ../img/icon-osx.png
.. _icon-osx: https://github.com/spatial-model-editor/spatial-model-editor/releases/latest/download/spatial-cli.dmg

.. |icon-windows| image:: ../img/icon-windows.png
.. _icon-windows: https://github.com/spatial-model-editor/spatial-model-editor/releases/latest/download/spatial-cli.exe

Use
---

It can be used to simulate a sbml model and save the results. For example, this would simulate the model from the file ``filename.xml`` for ten units of time, with 1 unit of time between images, and store the results in ``results.sme``:

.. code-block:: bash

    ./spatial-cli filename.xml 10 1 -o results.sme

The file ``results.sme`` can be opened in the GUI to see the simulation results.

An existing simulation can also be continued, for example this would simulate 5 more steps of length 1 and append the results to the existing simulation results in ``results.sme``:

.. code-block:: bash

    ./spatial-cli results.sme 5 1

If the output file is not specified it defaults to overwriting the input file.

Multiple time intervals can be specified as semicolon delimited lists, the same as in the GUI.
For example:

.. code-block:: bash

    ./spatial-cli results.sme 5;25;10 1;2.5;0.1

Command line parameters
-----------------------

.. code-block:: bash

    Spatial Model Editor CLI v1.1.1
    Usage: ./cli/spatial-cli [OPTIONS] file times image-intervals

    Positionals:
      file TEXT:FILE REQUIRED     The spatial SBML model to simulate
      times TEXT REQUIRED         The simulation time(s) (in model units of time)
      image-intervals TEXT REQUIRED
                                  The interval(s) between saving images (in model units of time)

    Options:
      -h,--help                   Print this help message and exit
      -s,--simulator ENUM:value in {dune->0,pixel->1} OR {0,1}=0
                                  The simulator to use: dune or pixel
      -o,--output-file TEXT       The output file to write the results to. If not set, then the input file is used.
      -n,--nthreads UINT:NONNEGATIVE=0
                                  The maximum number of CPU threads to use (0 means unlimited)
      -v,--version                Display the version number and exit
      -d,--dump-config            Dump the default config ini file and exit
      -c,--config                 Read an ini file containing simulation options


Using a config file
-------------------

To create an ini file with the default options

.. code-block:: bash

    ./spatial-cli -d > config.ini

You can then edit this file as desired, and use it when running a simulation

.. code-block:: bash

    ./spatial-cli filename.xml -c config.ini

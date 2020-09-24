Command Line Interface
======================

A Command Line Interface (CLI) is also provided for running long simulations:

*  |icon-linux|_ `linux <https://github.com/lkeegan/spatial-model-editor/releases/latest/download/spatial-cli>`_
*  |icon-osx|_ `macOS <https://github.com/lkeegan/spatial-model-editor/releases/latest/download/spatial-cli.dmg>`_
*  |icon-windows|_ `windows <https://github.com/lkeegan/spatial-model-editor/releases/latest/download/spatial-cli.exe>`_

.. |icon-linux| image:: ../img/icon-linux.png
.. _icon-linux: https://github.com/lkeegan/spatial-model-editor/releases/latest/download/spatial-cli

.. |icon-osx| image:: ../img/icon-osx.png
.. _icon-osx: https://github.com/lkeegan/spatial-model-editor/releases/latest/download/spatial-cli.dmg

.. |icon-windows| image:: ../img/icon-windows.png
.. _icon-windows: https://github.com/lkeegan/spatial-model-editor/releases/latest/download/spatial-cli.exe

Use
---

It can be used to simulate an existing model and save the resulting images:

.. code-block:: bash

    ./spatial-cli filename.xml

Command line parameters
-----------------------

.. code-block:: bash

   Spatial Model Editor CLI v0.9.2
   Usage: ./spatial-cli [OPTIONS] file

   Positionals:
     file TEXT:FILE REQUIRED     The spatial SBML model to simulate

   Options:
     -h,--help                   Print this help message and exit
     -c,--config                 Read an ini file containing simulation options
     -t,--time FLOAT:POSITIVE=100
                                 The simulation time (in model units of time)
     -i,--image-interval FLOAT:POSITIVE=1
                                 The interval between saving images (in model units of time)
     -s,--simulator ENUM:value in {dune->0,pixel->1} OR {0,1}=0
                                 The simulator to use: dune or pixel
     -n,--nthreads UINT:NONNEGATIVE=0
                                 The maximum number of CPU threads to use (0 means unlimited)
     -v,--version                Display the version number and exit
     -d,--dump-config            Dump the default config ini file and exit

Using a config file
-------------------

To create an ini file with the default options

.. code-block:: bash

    ./spatial-cli -d > config.ini

You can then edit this file as desired, and use it when running a simulation

.. code-block:: bash

    ./spatial-cli filename.xml -c config.ini

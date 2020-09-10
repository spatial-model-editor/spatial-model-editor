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

    ./spatial-cli filename [simulator] [simulation_time] [image_interval] [max_cpu_threads]

Command line parameters
-----------------------

* ``filename``
   * the SBML file containing the model to be simulated
* ``simulator``
   * this can be ``dune`` or ``pixel``
   * default: ``dune``
* ``simulation_time``
   * the total length of the simulation (in model units of time)
   * default: ``100``
* ``image_interval``
   * the interval (in model units of time) at which concentration images should be saved
   * default: ``10``
* ``max_cpu_threads``
   * the maximum number of cpu threads that should be used
   * default: ``0`` (which means unlimitied / use all available threads)
   * note: this parameter only applies to the ``pixel`` simulator

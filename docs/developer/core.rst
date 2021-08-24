Core
====

The core library implements all the functionality, which is then exposed to the user
via the GUI, Python library, or CLI.

It is split into four (somewhat interdependent) parts: :ref:`core-model`, :ref:`core-mesh`, :ref:`core-simulate` and :ref:`core-common`.

The public headers for each part are in ``/inc``, and the private implementation details are in ``/src``.

For each component X there is

* ``X.hpp`` the public interface
* ``X.cpp`` the private implementation
* ``X_t.cpp`` the tests
* ``X_bench.cpp`` the benchmarks (optional)

.. _core-model:

``sme::model``
--------------

Importing, exporting and editing spatial models.

.. toctree::

   model/inc
   model/src

.. _core-mesh:

``sme::mesh``
-------------

Constructing the simplified boundary lines and triangular mesh approximation to the geometry.

.. toctree::

   mesh/inc
   mesh/src

.. _core-simulate:

``sme::simulate``
-----------------

Simulating the model, either with Pixel or dune-copasi.

.. toctree::

   simulate/inc
   simulate/src

.. _core-common:

``sme::common``
---------------

Symbolic math, TIFF import/export, other utility functions.

.. toctree::

   common/inc
   common/src

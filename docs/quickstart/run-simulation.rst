Running a Simulation
====================

To simulate the spatial model, click on the "Simulate" tab,
specify the simulation time and desired interval between images, then click "Simulate".

The default simulation type is `DUNE`, a high-quality FEM solver,
which solves the PDE on the triangular meshed approximation of the geometry.

The other alternative is `Pixel`, a simple FTCS solver,
which solves the PDE on the grid formed by the pixels of the geometry image.

The simulation type can be chosen by clicking on `Tools->Set simulation type`.

The resulting average species concentrations are plotted as a function of time.
A snapshot of the spatial distribution is shown on the left,
and the slider below the plot can be used to change the time that is being displayed.
A 1d slice of the image as a function of time can also be generated.

.. figure:: img/simulation.apng
   :alt: screenshot showing simulation

   An example of a simple spatial simulation.

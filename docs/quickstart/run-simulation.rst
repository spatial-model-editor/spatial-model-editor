Running a Simulation
====================

To simulate the spatial model, click on the "Simulate" tab,
specify the simulation time and desired interval between images, then click "Simulate".

The default simulation type is `DUNE`, a high-quality FEM solver,
which solves the PDE on the triangular meshed approximation of the geometry.

The other alternative is `Pixel`, a simple `FTCS <https://en.wikipedia.org/wiki/FTCS_scheme>`_ solver,
which solves the PDE on the grid formed by the pixels of the geometry image.

The simulation type can be chosen by clicking on `Tools->Set simulation type`.

The resulting average species concentrations are plotted as a function of time.
A snapshot of the spatial distribution is shown on the left,
and the slider below the plot can be used to change the time that is being displayed.

.. note::
   The solvers have various parameters which can be customized via the Advanced->Simulation options menu.

.. note:: 
   You can find out more about the DUNE solver `here <../reference/dune.html>`_ and about the Pixel solver `here <../reference/pixel.html>`_.

Below the time series of the averaged species concentrations, you see three buttons, on the right of the time slider:
- `Slice image...`: This opens a new window in which you can choose a line through the geometry and plot the distribution of species along that line. In 3D, this represents a slice through the currently selected z-plane. You can choose vertical or horizontal slices, or draw a line yourself by holding the left mouse buton and dragging it over the image on the left side.
- `Export`: This opens a new dialog in which you can select various things to export from the simulation and save them on disk. You can export the entire simulation as a series of images (one for each time point), the averaged time series as a csv file, or you can export single time points to use as model initial conditions or to an image file to initialize the model from later on.
- `Display options`: This button opens a dialog in which the displayed plots can be customized to show only selected species, which is especially useful for analyzing complex systems with many species.

.. figure:: img/simulation.apng
   :alt: screenshot showing simulation

   An example of a simple spatial simulation.

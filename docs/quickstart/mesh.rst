Mesh generation
===============

Once the geometry has been created from an image and all compartments have been assigned a colour,
a meshed approximation to the geometry is automatically constructed.

The `Boundaries` tab shows the boundaries between compartments that have been automatically identified.
The number of points used for each boundary can be altered here to refine or coarsen
the boundary approximation.

The `Mesh` tab shows the generated triangular mesh with the currently selected compartment highlighted.
The maximum triangle area for each compartment can be altered here to refine or coarsen the mesh.

.. figure:: img/mesh.apng
   :alt: screenshot showing geometry mesh settings

   An example of changing the points used in the compartment boundaries, and changing the coarseness of the mesh.

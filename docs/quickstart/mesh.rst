Mesh generation
===============
The solution of the model equations will be created on a discretized version of the geometry.
Once the geometry has been created from an image and all compartments have been assigned a colour,
a meshed approximation to the geometry is automatically constructed.

The `Boundaries` tab shows the boundaries between compartments that have been automatically identified.
The number of points used for each boundary can be altered here to refine or coarsen the boundary approximation. 

The `Mesh` tab shows the generated triangular mesh with the currently selected compartment highlighted.
The maximum triangle area for each compartment can be altered here to refine or coarsen the mesh. More triangles will allow for a more accurate solution, but also needs more computational resources. This is especially relevant for 3D systems which have vastly more cells than 2D systems.
For more on how mesh generation works, see `the respective page in the reference documentation <reference/mesh.html>`_.

.. figure:: img/mesh.apng
   :alt: screenshot showing geometry mesh settings

   An example of changing the points used in the compartment boundaries, and changing the coarseness of the mesh.

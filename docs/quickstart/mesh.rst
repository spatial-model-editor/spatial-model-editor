Mesh generation
===============

Once the geometry has been created from an image, a mesh approximation to the geometry can be constructed for the solver.

The `Boundaries` tab shows the boundaries between compartments that have been automatically identified by the editor. The number of points used for each boundary can be altered here to refine or coarsen the boundary approximation.

Once the boundaries are chosen, the `Mesh` tab shows the generated triangular mesh for the currently selected compartment. The maximum triangle area can be altered here to refine or coarsen the mesh.

.. figure:: img/mesh.apng
   :alt: screenshot showing geometry mesh settings

   An example of changing the points used in the compartment boundaries, and changing the coarseness of the mesh.

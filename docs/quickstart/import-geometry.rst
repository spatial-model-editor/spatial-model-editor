Importing Geometry
==================

After importing a non-spatial model,
the next step is to import an image of the compartments in the model to define the geometry.

To do this, go to `Import->Geometry from image` or press ``Ctrl+I``.
Alternatively, go to `Import->Example geometry image` to use one of the built-in example images.

Then you can specify the dimensions of the geometry image, and optionally reduce the resolution
of the image or the number of colours to be used. The geometry image should be segmented such that each compartment has a unique colour.

Next, for each compartment in the model:

- click on the compartment name
- click on "Select compartment geometry..."
- then click on the desired part of the image.

.. figure:: img/geometry.apng
   :alt: screenshot showing geometry settings

   An example of importing a geometry image, and then assigning each compartment to a region in the image.

Importing Geometry
==================

The first step when creating a new model is defining the domain in which it shall be simulated.
The basic structure can be imported from an image file (e.g. png for 2D domains, or tiff files for 3D domains). You can also import the geometry of an existing model, or a 3D mesh from a Gmsh file. A number of example images for defining model geometries are also available.

To do this, go to `Import->Geometry from image` or press ``Ctrl+I``. By selecting `Import->Geometry from model` or pressing ``Ctrl+G`` you can import the geometry of an existing model,
and with `Import->Geometry from gmsh...` or ``Ctrl+Shift+G`` you can import a Gmsh mesh file.
Alternatively, go to `Import->Example geometry image` to use one of the built-in example images.
For 3D geometries, the z-slice that is displayed is set to zero by default. Play around with the slider to vertically move through the geometry.

.. note::
   Images that should be used as a model geometry with multiple compartments must be clearly segmented. Each compartment must have a unique color.

To customize the dimensions of the geometry image, change the resolution of the image or the number of colors to be used, go to `Tools -> Edit geometry image`. This brings up the same editor dialog that is shown when importing an image.

.. figure:: img/geometry.apng
   :alt: screenshot showing geometry image import

   An example of importing a geometry image

Next, we will define the different compartments of the geometry.

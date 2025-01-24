Importing Geometry
==================

The first step when creating a new model is defining the domain in which it shall be simulated. 
The basic structure can be imported from an image file (png for 2D domains for tiff files for 3D domains). You can also import the geometry of an existing model. A number of example images for defining model geometries are also available. 

To do this, go to `Import->Geometry from image` or press ``Ctrl+I``. By selecting `Import->Geometry from model` or pressing ``Ctrl+G`` you can import the geometry of an existing model.
Alternatively, go to `Import->Example geometry image` to use one of the built-in example images.
After this, customize the dimensions of the geometry image, change the resolution of the image or the number of colours to be used, click on `Tools -> Edit geometry image`. For 3D geometries, the z-axis is set to zero by default. Play around with the slider to vertically move through the geometry.

..note:: Images that should be used as a model geometry with multiple compartments must be clearly segmented. Each compartment must have a unique colour.

To change the geometry afterwards, go to `Tools -> Edit geometry image`. This will bring up the editor dialog again. 

.. figure:: img/geometry.apng
   :alt: screenshot showing geometry image import

   An example of importing a geometry image

Next, we will define the different compartments of the geometry.
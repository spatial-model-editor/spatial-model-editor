Assign Compartments
===================

After importing a geometry image, each compartment can be assigned a region in the image:

* In the compartment section of the GUI, click on `Add` to create a new compartment. Choose a name and hit enter to confirm it.
* click on "Select compartment geometry..."
* then click on the desired part of the image.
* repeat this until all compartments you want to have are assigned.
The compartment geometry will then be made up from all of the pixels in the image of the chosen colour.

Not every region of the image has to be assigned a compartment, but every compartment needs to be assigned to a region of the image.
When compartments have been assigned that have a common boundary, this boundary will be recognized automatically and be listed below the 'Compartments' tab under 'Membranes'.

The model equations will be defined within these compartments and on their interfaces.

.. figure:: img/assign-compartments.apng
   :alt: screenshot showing each compartment geometry being assigned

   An example of assigning each compartment to a region in the geometry image.

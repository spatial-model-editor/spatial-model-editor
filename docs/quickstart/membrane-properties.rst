Membrane Properties
===================

If two compartments touch each other, the GUI identifies their common boundary as a *membrane*. 

If the model contains a reaction involving species from both these compartments, it will take place on the membrane.

By default, membranes are assumed to be impermeable (the same as any other boundary, i.e. the zero flux Neumann boundary condition is applied on both sides.)

.. figure:: img/membrane.png
   :alt: screenshot showing membrane settings

   An example of a membrane automatically identified between two compartments.

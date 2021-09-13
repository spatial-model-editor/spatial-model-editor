Rescale Reactions
=================

A non-spatial ODE model contains reaction rates that specify
a rate of change of species **total amount** in a given compartment.

In a PDE model, we have

*  compartment reactions: rate of change of species **concentration**
*  membrane reactions: rate of species **amount crossing unit area** of the membrane

The last step in importing a non-spatial model is to rescale the reactions appropriately,
by clicking on "rescale reactions" in the bottom right of the window.

This opens a guided workflow that automatically rescales the image depth and the reaction
rate expressions, with the goal of constructing a valid PDE model that initially reproduces
the behaviour of the imported ODE model, in the following limit of the spatial model:

- no initial spatial dependence of species concentrations
- infinitely fast diffusion
- consistent compartment volumes between ODE model and PDE model geometry

.. figure:: img/rescale-reactions.apng
   :alt: screenshot showing the automatic rescaling of non-spatial reactions

   An example of the automatic rescaling of image depth and reaction rates when importing a non-spatial model

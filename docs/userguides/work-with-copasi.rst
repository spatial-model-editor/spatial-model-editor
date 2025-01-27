Work with SBML and COPASI models
=================================

You can use SME to turn non-spatial ODE models created with COPASI or SBML into spatial models. This guide will show you how to do this. If you haven't already, start by reading through the `gui overview <gui-overview.html>`_, because many of the workflows described there are useful here, too.

Importing a Model
-----------------

To import an existing SBML or COPASI model,
go to `File->Open` or press ``Ctrl+O``.

There are also some built-in example models,
to open one of these go to `File->Open example model`

.. figure:: img/model.apng
   :alt: screenshot showing built-in example model

   One of the built-in example models: `very-simple-model.xml`

Rescale Reactions
-----------------

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



Reactions
---------

A non-spatial ODE model contains reaction rates that specify
a rate of change of species **total amount** in a given compartment.

In a PDE model, we have two kinds of reactions

* Compartment reactions
   * take place everywhere within a compartment
   * units: rate of change of species **concentration**
* Membrane reactions
   * take place on the membrane between two compartments
   * units: rate of species **amount crossing unit area** of the membrane

When editing a reaction,
an image of the location is displayed on the left,
and the units are displayed below the reaction rate in the bottom right.

.. figure:: img/reactions.apng
   :alt: screenshot of the reactions in a model

   Editing the reactions in a spatial model.

Work with SBML and COPASI models 
=================================

This guide will show you how to use SME to turn non-spatial ODE models created with COPASI or SBML into spatial models. If you haven't already, start by reading through the `getting started section <../quickstart/get-started.html>`_, because many of the workflows described there are useful here, too.

Importing a Model
-----------------

To import an existing SBML or COPASI model,
go to `File->Open` or press ``Ctrl+O``, like shown below:

.. figure:: img/model.apng
   :alt: screenshot showing built-in example model

   Open an existing model in SME

Since the imported model is non-spatial, we need to define the geometry on which it should run next. To learn how to define a geometry with different compartments, see the `geometry section in the getting-started guide <../quickstart/import-geometry.html>`_ and the `respective section on assiging compartments <../quickstart/assign-compartments.html>`_. 

If you want to create your own geometry, please refer to `the respective userguide <./how-to-create-your-own-geometry.html>`_.

Define or change reactions
--------------------------

A non-spatial ODE model contains reaction rates that specify
a rate of change of species **total amount** in a given compartment.

In a PDE model, we have two kinds of reactions

* Compartment reactions
   * take place everywhere within a compartment
   * units: rate of change of species **concentration**
* Membrane reactions
   * take place on the membrane between two compartments
   * units: rate of species **amount crossing unit area** of the membrane

For more on the the formulae involved, see the `mathematical formulation <../reference/maths.html>`_.

When editing a reaction, an image of the location is displayed on the left,
and the units are displayed below the reaction rate in the bottom right.

.. figure:: ../quickstart/img/reactions.apng
   :alt: screenshot of the reactions in a model

   Editing the reactions in a spatial model.

Since the system already contains a number of reactions from the imported model, we need to assign them to the right compartment here first. When this is done, we need to add the membrane reactions on the interfaces between compartments (if there are any). See `here <../quickstart/reactions.html>`_ for more details on how to define reactions in SME.

Rescale Reactions
-----------------

The next step in importing a non-spatial model is to rescale the existing reactions appropriately,
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


After this workflow has been completed, you can treat the model like any other spatial model in SME.
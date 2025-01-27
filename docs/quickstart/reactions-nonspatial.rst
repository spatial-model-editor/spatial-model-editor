Reactions
=========

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

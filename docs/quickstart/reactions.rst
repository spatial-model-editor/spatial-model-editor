Reactions
=========

A non-spatial ODE model contains reaction rates that specify
a rate of change of species **total amount** in a given compartment.

In a PDE model, we have

- compartment reactions: rate of change of species **concentration**
- membrane reactions: rate of species **amount crossing unit area** of the membrane

Automatic Rescaling
-------------------

On import, we automatically divide the reaction rate by the corresponding compartment volume or
membrane area. If a reaction is then moved from a compartment to a membrane, it is rescaled
by multiplying by the compartment volume and dividing by the membrane area. Similarly, if
moved from a membrane to a compartment, it is multiplied by the membrane area and divided
by the compartment volume.

The goal of this automatic rescaling is for the PDE model to initially reproduce
the behaviour of the imported ODE model, in the following limit of the spatial model:

- no initial spatial dependence of species concentrations
- infinitely fast diffusion
- consistent compartment volumes between ODE model and PDE model geometry

.. tip::
   Typically reaction rates in a PDE model would not explicitly depend on
   compartment volumes or membrane areas. After the model is imported, the geometry
   constructed, and all reactions are assigned to their desired locations, it will often
   make sense to replace any remaining dependence on volumes or areas by an equivalent
   fixed rescaling factor.

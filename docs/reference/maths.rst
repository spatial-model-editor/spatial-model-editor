Maths
=====

Reaction-Diffusion
------------------

The system of PDEs that we simulate in each compartment is the three-dimensional reaction-diffusion equation:

.. math::

   \frac{\partial c_s}{\partial t} = D_s \left( \frac{\partial^2}{\partial x^2} + \frac{\partial^2}{\partial y^2} + \frac{\partial^2}{\partial z^2} \right) c_s + R_s

where

* :math:`c_s` is the concentration of species :math:`s` at position :math:`(x, y, z)` and time :math:`t`
* :math:`D_s` is the diffusion constant for species :math:`s`
* :math:`R_s` is the reaction term for species :math:`s`

and we assume that

* the diffusion constant :math:`D_s` is a scalar that does not vary with position or time
* the reaction term :math:`R_s` is a function that can depend on the concentrations of other species in the model, but only locally, i.e. the concentrations at the same spatial coordinate.

Compartment Reactions
---------------------

Compartment reaction terms correspond to the :math:`R_s` term in the reaction-diffusion equation,
and describe the rate of change of species concentration with time.
They are evaluated at every point inside the compartment

Membrane reactions
------------------

Membrane reactions are reactions that occur on the membrane between two compartments,
and describe the species amount that crosses the membrane per unit membrane area per unit time.

Boundary Conditions
-------------------

All boundaries have "zero-flux" Neumann boundary conditions,
whether they are boundaries between two compartments or boundaries between a compartment and the outside (except for the flux caused by any membrane reactions).

Maths
=====

Reaction-Diffusion
------------------

The system of PDEs that we simulate is the two-dimensional reaction-diffusion equation:

.. math::

   \frac{\partial c_s}{\partial t} = D_s \left( \frac{\partial^2}{\partial x^2} + \frac{\partial^2}{\partial y^2} \right) c_s + R_s

where

* :math:`c_s` is the concentration of species :math:`s` at position :math:`(x, y)` and time :math:`t`
* :math:`D_s` is the diffusion constant for species :math:`s`
* :math:`R_s` is the reaction term for species :math:`s`

and we assume that

* the diffusion constant :math:`D_s` is a scalar that does not vary with position or time
* the reaction term :math:`R_s` is a function that can depend on the concentrations of other species in the model, but only locally, i.e. the concentrations at the same spatial coordinate.

Boundary Conditions
-------------------

All boundaries have "zero-flux" Neumann boundary conditions, whether they are boundaries between two compartments or boundaries between a compartment and the outside.

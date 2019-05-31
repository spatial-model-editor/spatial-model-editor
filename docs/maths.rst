Maths
=====

Some information on the mathematical formulation of the spatial models.

Reaction-Diffusion
------------------

The PDE to be simulated is a simple version of the two-dimensional reaction-diffusion equation:

.. math::

   \frac{\partial c_i}{\partial t} = D \left( \frac{\partial^2}{\partial x^2} + \frac{\partial^2}{\partial y^2} \right) c_i + R_i

where

* :math:`c_i` is the concentration of species :math:`i` at position :math:`(x, y)` and time :math:`t`
* :math:`D` is the diffusion constant for species :math:`i`
* :math:`R_i` are the reaction terms for species :math:`i`

Assumptions:

* The diffusion constant is a scalar that does not vary with position or time
* The reaction terms can depend on the concentrations of other species in the model, but only locally, i.e. the concentrations at the same spatial coordinate.
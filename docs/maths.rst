Maths
=====

Some information on the mathematical formulation of the spatial models.

Reaction-Diffusion
------------------

The PDE to be simulated is a simple version of the two-dimensional reaction-diffusion equation:

.. math::

   \frac{\partial c_s}{\partial t} = D \left( \frac{\partial^2}{\partial x^2} + \frac{\partial^2}{\partial y^2} \right) c_s + R_s

where

* :math:`c_s` is the concentration of species :math:`s` at position :math:`(x, y)` and time :math:`t`
* :math:`D` is the diffusion constant for species :math:`s`
* :math:`R_i` are the reaction terms for species :math:`s`

Assumptions:

* The diffusion constant is a scalar that does not vary with position or time
* The reaction terms can depend on the concentrations of other species in the model, but only locally, i.e. the concentrations at the same spatial coordinate.

Boundary Conditions
-------------------

todo

Spatial Discretization
----------------------

To solve the PDE numerically, space is discretized using a uniform, linear grid with spacing :math:`\Delta`. Now the concentration is defined as a 2d array of values :math:`c(i,j)`, where the value with index :math:`(i,j)` corresponds to the concentration at the spatial point :math:`(x = i\Delta, y = j \Delta)`.

An approximation to the Laplacian on this grid is given by

.. math::

   \left( \frac{\partial^2}{\partial x^2} + \frac{\partial^2}{\partial y^2} \right) c(i,j) \simeq \left[ c(i+1,j) + c(i,j+1) - 4 c(i,j) + c(i-1,j) + c(i,j-1) \right] / \Delta^2

and inserting this approximation converts the PDE into a system of coupled ODEs.

Time Integration
----------------

To numerically integrate a system of coupled ODEs, a numerical integration scheme can be used to convert the equations into a set of algebraic relations involving :math:`c(t+\delta t)` and :math:`c(t)`, which can then be solved to find :math:`c(t+\delta t)` in terms of the current value :math:`c(t)`.

In general stable numerical integration schemes are implicit, and result in a very large sparse non-linear system to be solved at each timestep.

.. warning::
   Currently only forwards Euler integration is implemented, which is both numerically unstable and only accurate to first order in :math:`\delta t`.
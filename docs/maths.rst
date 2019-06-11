Maths
=====

Reaction-Diffusion
------------------

The PDE to be simulated is a simple version of the two-dimensional reaction-diffusion equation:

.. math::

   \frac{\partial c_s}{\partial t} = D \left( \frac{\partial^2}{\partial x^2} + \frac{\partial^2}{\partial y^2} \right) c_s + R_s

where

* :math:`c_s` is the concentration of species :math:`s` at position :math:`(x, y)` and time :math:`t`
* :math:`D` is the diffusion constant for species :math:`s`
* :math:`R_s` is the reaction term for species :math:`s`

Assumptions:

* The diffusion constant is a scalar that does not vary with position or time
* The reaction terms can depend on the concentrations of other species in the model, but only locally, i.e. the concentrations at the same spatial coordinate.

Boundary Conditions
-------------------

Currently all boundaries are treated in the same way, whether they are boundaries between two compartments or boundaries between the outside and a compartment.

Neumann
^^^^^^^
The default choice is the "zero-flux" boundary condition, which is a Neumann boundary condition where the flux out of the boundary is set to the fixed value of zero. This is implemented upon discretization by setting the concentration in the neighbouring pixel that lies outside the compartment boundary to be equal to the concentration in the boundary pixel value.

Dirichlet
^^^^^^^^^
Alternatively, the value of the concentration at the boundaries can be set to zero, which is a Dirichlet boundary condition. This is implemented upon discretization by setting the concentration in the neighbouring pixel that lies outside the compartment boundary to be equal to zero.

Spatial Discretization
----------------------

To solve the PDE numerically, space is discretized using a uniform, linear grid with spacing :math:`\Delta`. Now the concentration is defined as a 2d array of values :math:`c(i,j)`, where the value with index :math:`(i,j)` corresponds to the concentration at the spatial point :math:`(x = i\Delta, y = j \Delta)`.

An approximation to the Laplacian on this grid is given by

.. math::

   \left( \frac{\partial^2}{\partial x^2} + \frac{\partial^2}{\partial y^2} \right) c(i,j) \simeq \left[ c(i+1,j) + c(i,j+1) - 4 c(i,j) + c(i-1,j) + c(i,j-1) \right] / \Delta^2

and inserting this approximation converts the PDE into a system of coupled ODEs.

Compartments
^^^^^^^^^^^^

Each compartment is discretized, with boundary conditions applied.

Membranes
^^^^^^^^^

todo


Time Integration
----------------

To numerically integrate a system of coupled ODEs, a numerical integration scheme can be used to convert the equations into a set of algebraic relations involving :math:`c(t+\delta t)` and :math:`c(t)`, which can then be solved to find :math:`c(t+\delta t)` in terms of the current value :math:`c(t)`.

In general stable numerical integration schemes are implicit, and result in a very large sparse non-linear system to be solved at each timestep. Here we use the explicit forwards Euler scheme which is easy to implement, but numerically unstable and only accurate to first order in :math:`\delta t`.
Maths
=====

Reaction-Diffusion
------------------

The system of PDEs that we simulate in each compartment is the three-dimensional reaction-diffusion equation:

.. math::

   S_s \frac{\partial c_s}{\partial t}
   =
   \nabla \cdot \left(
     D_{s,s} \nabla c_s
     + \sum_{r \ne s} D_{s,r} \nabla c_r
   \right)
   + R_s

where

* :math:`c_s` is the concentration of species :math:`s` at position :math:`(x, y, z)` and time :math:`t`
* :math:`D_{s,s}` is the (possibly spatially varying) scalar self-diffusion constant for species :math:`s`
* :math:`D_{s,r}` (:math:`r \ne s`) are optional cross-diffusion coefficients for species :math:`s`, multiplying gradients of other species :math:`r` in the same compartment
* :math:`R_s` is the reaction term for species :math:`s`
* :math:`S_s` is the species storage coefficient (dimensionless, non-negative, default :math:`1`)

If no cross-diffusion terms are defined, all :math:`D_{s,r}` with :math:`r \ne s` are zero and this reduces to the standard reaction-diffusion equation.

and we assume that

* each diffusion coefficient :math:`D_{s,r}` is isotropic (a scalar, not a tensor)
* the reaction term :math:`R_s` is a function that can depend on the concentrations of other species in the model, but only locally, i.e. the concentrations at the same spatial coordinate.

When :math:`S_s = 0`, the equation becomes an algebraic constraint rather than a time-evolution equation:

.. math::

   0
   =
   \nabla \cdot \left(
     D_{s,s} \nabla c_s
     + \sum_{r \ne s} D_{s,r} \nabla c_r
   \right)
   + R_s

The concentration of such a species is not integrated in time, but is instead determined at each timestep
by satisfying this constraint. See the :doc:`pixel` documentation for details on how the Pixel solver handles this case.

For :math:`S_s > 0`, the equation can equivalently be written as

.. math::

   \frac{\partial c_s}{\partial t}
   =
   \frac{1}{S_s}\left(
     \nabla \cdot \left(
       D_{s,s} \nabla c_s
       + \sum_{r \ne s} D_{s,r} \nabla c_r
     \right)
     + R_s
   \right)

If all diffusion coefficients are constant in space, this reduces to
:math:`\frac{1}{S_s}\left(D_{s,s}\nabla^2 c_s + \sum_{r \ne s} D_{s,r}\nabla^2 c_r + R_s\right)`.

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

Diffusion
=========

Model
-----

The example model `single-compartment-diffusion <https://github.com/lkeegan/spatial-model-editor/blob/master/src/core/resources/models/single-compartment-diffusion.xml>`_ is a single compartment that contains two species: 'fast' and 'slow', each with the same analytic initial distribution

.. math::

   c_s(t=0) = e^{-((x-48)^2+(y-48)^2)/36}

The two species have different diffusion coefficients: :math:`D=1cm^2/s` for species 'slow', and :math:`D=3cm^2/s` for species 'fast', and the model contains no reactions.

Analytic solution
-----------------

For this system without reactions, we are simulating the two-dimensional diffusion equation,

.. math::

   \frac{\partial c_s}{\partial t} = D_s \left( \frac{\partial^2}{\partial x^2} + \frac{\partial^2}{\partial y^2} \right) c_s

where

* :math:`c_s` is the concentration of species :math:`s` at position :math:`(x, y)` and time :math:`t`
* :math:`D_s` is the diffusion constant for species :math:`s`

For the initial condition :math:`c_s(t=0) = \delta(x)\delta(y)`, the analytic solution at time `t` of this equation is the `heat kernel <https://en.wikipedia.org/wiki/Heat_kernel>`_:

.. math::

   c_s(t) = \frac{1}{4 \pi D_s t}e^{-(x^2+y^2)/(4 D_s t)}

and a solution for our model can then be found by convolving this expression with our initial condition, to give

.. math::

   c_s(t) = \frac{t}{t+t_0}e^{-((x-48)^2+(y-48)^2)/(4 D_s (t+t_0))}

where :math:`t_0 = 9/D_s`. Note that this solution ignores boundary effects, so will not be valid at late times or close to the compartment boundary.

The total amount of species in the compartment is a conserved quantity

.. math::

   \int_{-\infty}^{\infty} \int_{-\infty}^{\infty} c_s(t) dx dy = 36 \pi

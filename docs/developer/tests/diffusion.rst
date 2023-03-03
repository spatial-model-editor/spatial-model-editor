Diffusion
=========

2d Model
--------

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

and a solution for our initial condition can then be found with an overall rescaling and a shift in t:

.. math::

   c_s(t) = \frac{t_0}{t+t_0}e^{-((x-48)^2+(y-48)^2)/(4 D_s (t+t_0))}

where :math:`t_0 = 9/D_s`. Note that this solution ignores boundary effects, so will not be valid at late times or close to the compartment boundary.

The total amount of species in the compartment is a conserved quantity,

.. math::

   \int_{-\infty}^{\infty} \int_{-\infty}^{\infty} c_s(t) dx dy = 36 \pi

and this is also valid at late times, since our zero flux Neumann boundary conditions also conserve the amount of species in the compartment.

3d Model
--------

The example model `single-compartment-diffusion-3d <https://github.com/lkeegan/spatial-model-editor/blob/master/src/core/resources/models/single-compartment-diffusion-3d.xml>`_ is a single compartment that contains two species: 'fast' and 'slow', each with the same analytic initial distribution.

Key differences from the 2d example above are that the origin of the geometry lies in the centre of a 100cm^3 cube, with initial species concentration given by:

.. math::

   c_s(t=0) = e^{-(x^2+y^2+z^2)/36}

and the heat kernel in 3d is given by:

.. math::

   c_s(t) = \frac{1}{(4 \pi D_s t)^{3/2}}e^{-(x^2+y^2+z^2)/(4 D_s t)}

which gives the solution at time t (again ignoring boundary effects):

.. math::

   c_s(t) = (\frac{t_0}{t+t_0})^{3/2}e^{-(x^2+y^2+z^2)/(4 D_s (t+t_0))}

where :math:`t_0 = 9/D_s`, with total amount of species in the compartment a conserved quantity:

.. math::

   \int_{-\infty}^{\infty} \int_{-\infty}^{\infty} \int_{-\infty}^{\infty} c_s(t) dx dy dz = (36 \pi)^{3/2}

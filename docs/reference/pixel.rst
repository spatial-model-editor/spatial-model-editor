Pixel simulator
===============

Pixel is an alternative PDE solver which uses the simple
`FTCS <https://en.wikipedia.org/wiki/FTCS_scheme>`_ method to solve
the PDE using the pixels of the geometry image as the grid.

Simulation options
------------------

The default settings should work well in most cases,
but if desired they can be adjusted by going to `Advanced->Simulation options`

.. figure:: img/pixel_options.png
   :alt: Pixel simulation options

   The simulation options that can be used to fine tune-the Pixel solver.

* Integrator
   * the explicit Runge-Kutta integration scheme used for time integration
   * default: 2nd order Heun scheme, with embedded 1st order error estimate
   * a higher order scheme may be more efficient if the maximum allowed error is very small
   * see the :ref:`time-integration` section for more information on the integrators
* Max relative local error
   * the maximum relative error allowed on the concentration of any species at any pixel
   * default: 0.005
   * local means the estimated error for a single timestep, at a single point
   * relative means each error estimate is divided by the species concentration
   * making this number smaller makes the simulation more accurate, but slower
* Max absolute local error
   * the maximum error allowed on the concentration of any species at any pixel
   * default: infinite
   * local means the estimated error for a single timestep, at a single point
   * absolute means the error estimate is not normalised by the species concentration
   * making this number smaller makes the simulation more accurate, but slower
* Max timestep
   * the maximum allowed timestep
   * default: infinite
* Multithreading
   * if enabled, multiple CPU threads can be used
   * default: disabled
   * enabling this can make simulations of large models run faster
   * however it can also make small models run slower
* Max CPU threads
   * limit the maximum number of CPU threads to be used
   * default: unlimited
* Symbolic optimization
   * factor out common subexpressions when constructing the reaction terms
   * default: enabled
* Compiler optimization level
   * how much optimization is done when compiling the reaction terms
   * default: 3

Spatial discretization
----------------------

Space is discretized using a linear grid with spacing :math:`\delta x`, :math:`\delta y`, :math:`\delta z`.
The concentration is defined as a 3d array of values :math:`c_{i,j,k}`,
where the value with index :math:`(i,j,k)` corresponds to the concentration
at the spatial point :math:`(x = i \delta x, y = j \delta y, z = k \delta z)`.

For diffusion constants that vary in space, the diffusion term is written in conservative form

.. math::

   \frac{\partial c}{\partial t} = \nabla \cdot \left( D(\mathbf{x}) \nabla c \right) + R

and discretized using face fluxes with arithmetic averaging of :math:`D` at neighbouring voxels.
For one species in voxel :math:`(i,j,k)`:

.. math::

   \begin{eqnarray}
   \left[\nabla \cdot \left( D \nabla c \right)\right]_{i,j,k}
   & = & \frac{D^x_{i+1/2,j,k}(c_{i+1,j,k}-c_{i,j,k}) - D^x_{i-1/2,j,k}(c_{i,j,k}-c_{i-1,j,k})}{\delta x^2} \\
   & & + \frac{D^y_{i,j+1/2,k}(c_{i,j+1,k}-c_{i,j,k}) - D^y_{i,j-1/2,k}(c_{i,j,k}-c_{i,j-1,k})}{\delta y^2} \\
   & & + \frac{D^z_{i,j,k+1/2}(c_{i,j,k+1}-c_{i,j,k}) - D^z_{i,j,k-1/2}(c_{i,j,k}-c_{i,j,k-1})}{\delta z^2} \\
   & & + \mathcal{O}(\delta x^2)+\mathcal{O}(\delta y^2)+\mathcal{O}(\delta z^2)
   \end{eqnarray}

with face diffusion constants

.. math::

   D^x_{i+1/2,j,k} = \frac{D_{i,j,k} + D_{i+1,j,k}}{2}

and similarly in :math:`y,z` (and for the minus faces).

This has :math:`\mathcal{O}(\delta x^2)+\mathcal{O}(\delta y^2)+\mathcal{O}(\delta z^2)` discretisation errors.
Inserting this approximation into the reaction-diffusion equation converts the PDE into a system of coupled ODEs.

If :math:`D` is spatially constant, this reduces to the usual central-difference Laplacian form.

Step-by-step derivation
^^^^^^^^^^^^^^^^^^^^^^^

For one species, start from

.. math::

   \frac{\partial c}{\partial t} = \nabla \cdot (D \nabla c) + R

1. Integrate over one voxel :math:`V_{i,j,k}`

Assumptions:

* :math:`D(\mathbf{x})` is isotropic (scalar), but may vary in space
* :math:`R` is local in space (depends on the state at the same point)

.. math::

   \int_{V_{i,j,k}}\frac{\partial c}{\partial t}\,dV
   =
   \int_{V_{i,j,k}}\nabla\cdot(D\nabla c)\,dV
   +
   \int_{V_{i,j,k}}R\,dV

2. Apply the divergence theorem to the diffusion term

.. math::

   \int_{V_{i,j,k}}\nabla\cdot(D\nabla c)\,dV
   =
   \int_{\partial V_{i,j,k}} (D\nabla c)\cdot \mathbf{n}\,dS

This is the sum of six face flux integrals (:math:`x^\pm, y^\pm, z^\pm`).

3. Replace volume/face integrals by midpoint (cell-centered) approximations

Assumption: uniform Cartesian grid with voxel spacings :math:`\delta x,\delta y,\delta z`.

Define cell-center values :math:`c_{i,j,k}\approx c(\mathbf{x}_{i,j,k})` and
:math:`D_{i,j,k}\approx D(\mathbf{x}_{i,j,k})`.
Then

.. math::

   \int_{V_{i,j,k}}\frac{\partial c}{\partial t}\,dV
   \approx
   \delta x\,\delta y\,\delta z\;\frac{d c_{i,j,k}}{dt}

.. math::

   \int_{x^+}(D\nabla c)\cdot\mathbf{n}\,dS
   \approx
   \delta y\,\delta z\;F^x_{i+1/2,j,k}

and similarly for the other faces.

4. Discretize face-normal gradients by central differences

.. math::

   F^x_{i+1/2,j,k}
   \approx
   D^x_{i+1/2,j,k}\frac{c_{i+1,j,k}-c_{i,j,k}}{\delta x},
   \qquad
   F^x_{i-1/2,j,k}
   \approx
   D^x_{i-1/2,j,k}\frac{c_{i,j,k}-c_{i-1,j,k}}{\delta x}

and similarly for the minus face and for :math:`y,z`.

5. Approximate face diffusion constants from neighbouring cell values

Assumption: face diffusion is approximated by arithmetic interpolation of neighbouring voxel values:

.. math::

   D^x_{i+1/2,j,k} = \frac{D_{i,j,k}+D_{i+1,j,k}}{2}

(and similarly for :math:`y,z`).

6. Assemble the semi-discrete ODE (method of lines)

Substituting the face fluxes into the surface-balance form and dividing by
:math:`\delta x\,\delta y\,\delta z` gives

.. math::

   \begin{aligned}
   \frac{d c_{i,j,k}}{dt} =\;&
   \frac{D^x_{i+1/2,j,k}(c_{i+1,j,k}-c_{i,j,k}) - D^x_{i-1/2,j,k}(c_{i,j,k}-c_{i-1,j,k})}{\delta x^2} \\
   &+ \frac{D^y_{i,j+1/2,k}(c_{i,j+1,k}-c_{i,j,k}) - D^y_{i,j-1/2,k}(c_{i,j,k}-c_{i,j-1,k})}{\delta y^2} \\
   &+ \frac{D^z_{i,j,k+1/2}(c_{i,j,k+1}-c_{i,j,k}) - D^z_{i,j,k-1/2}(c_{i,j,k}-c_{i,j,k-1})}{\delta z^2}
   + R_{i,j,k}
   \end{aligned}

where :math:`R_{i,j,k}` is the cell-centered reaction term.

7. Local truncation error

Assumption: :math:`c` and :math:`D` are sufficiently smooth.
Each directional term is second-order accurate on the uniform grid, so the spatial error is
:math:`\mathcal{O}(\delta x^2)+\mathcal{O}(\delta y^2)+\mathcal{O}(\delta z^2)`.

8. Boundary handling

Assumption: zero-flux Neumann boundaries.
Implementation-wise, an out-of-domain neighbour index is replaced by the boundary voxel itself, which makes the corresponding face gradient (and therefore face flux) zero.

.. _time-integration:

Time integration
----------------

Time integration is performed using explicit Runge-Kutta integrators. Compared to implicit integrators, they are easier to implement and offer better performance (for the same timestep). However they become unstable if the timestep :math:`h` is made too large, so in practice they can end up being slower than implicit methods for stiff problems, where the timestep is forced to be very small to maintain stability.

Integrators differ in their:

* order of truncation error
* order of embedded error estimate (if any)
* number of stages (i.e. cost of a step)
* region of stability (can be increased by adding more stages)
* memory requirements

Implemented integrators:

* Euler
   * 1st order solution
   * no error estimate
   * 1 stage
   * see e.g. https://en.wikipedia.org/wiki/Euler_method
* Embedded Heun / modified Euler
   * 2nd order solution
   * 1st order error estimate
   * 2 stages
   * see e.g. eq (2.15) of https://doi.org/10.1016/0021-9991(88)90177-5
* Embedded Shu-Osher
   * 3rd order solution
   * 2nd order error estimate
   * 3 stages
   * see eq (2.17) of https://doi.org/10.1016/0021-9991(88)90177-5
* RK4(3)5[3S*]
   * 4th order solution
   * 3rd order error estimate
   * 5 stages
   * see alg.6 & tab.6 of https://doi.org/10.1016/j.jcp.2009.11.006

.. figure:: img/convergence.png
   :alt: convergence of the RK integrators

   An example of the convergence of the included RK integrators: relative error of the solution at a particular pixel as a function of the stepsize.

These integrators have three sources of error:

* Round-off error due to finite precision
   * mostly only relevant for high order solvers: not relevant here
* Truncation error due to finite order of integration scheme
   * we are generally forced by the diffusion term to make the timestep small to maintain stability
   * also no benefit from making the time integration errors significantly smaller than the spatial discretisation errors
   * so this is also typically not a concern
* Numerical instability of integrator
   * a problem when ODEs become stiff, e.g. high rate of diffusion, stiff reaction terms
   * avoiding these instabilities is our main concern

Adaptive timestep
-----------------

We use the embedded lower order solution to estimate the error at each timestep, and use this to adapt the stepsize during the integration:

* RK gives us a pair of :math:`u_{n+1}^{(p)} = u_{n} + \mathcal{O}(h^{p+1})` solutions
* difference between :math:`p, p-1` solutions gives local error of order :math:`\mathcal{O}(p)`
* to get the relative error we divide this by :math:`c = ( |c_{n+1}| + |c_{n}| + \epsilon)/2`
* we use the average of the old and new concentration, plus a small constant, to avoid dividing by zero
* we do this for all species, compartments and spatial points, and take the maximum value
* if this error is larger than the desired value, the whole step is discarded
* the new timestep is given by :math:`0.98 dt_{old} (err_{desired}/err_{measured})^{1/p}`
* the 0.98 factor is slightly less than 1 to account for the higher order terms that are neglected here
* it is better to have a slightly smaller timestep than to have to repeat the whole step

.. figure:: img/embedded.png
   :alt: difference between solutions of different order from embedded schemes

   An example of the difference between order p and order p-1 solutions from embedded schemes as a function of the stepsize. This quantity is a measure of the local integration error, and scales like :math:`h^p`

Maximum timestep
----------------

For the Euler method, we don't have an embedded lower order solution from which we can estimate of the error,
so we can't automatically adjust the stepsize.
However, if we ignore the reaction terms,
there is an analytic upper bound on the size of timestep that can be used for Euler (see p125 of https://doi.org/10.1017/CBO9780511781438 ),
above which the system becomes unstable:

.. math::

   \delta t \leq  \frac{1}{2 D_{\max} \left(\frac{1}{\delta x^2}+\frac{1}{\delta y^2}+\frac{1}{\delta z^2}\right)}

where :math:`D_{\max}` is the largest diffusion constant value over all voxels for the species.

So if the user selects a timestep larger than this,
the simulator automatically reduces it to the above value to avoid the system becoming unstable.
Note that the system can still become unstable if the reaction terms are stiffer than the diffusion terms.

.. figure:: img/runtime.png
   :alt: runtime of the RK integrators

   An example of the runtime of the RK integrators as a function of the relative error on the final solution. The higher order integrators offer better performance if a very accurate solution is required, but at lower accuracy the lower order integrators are much faster.

Boundary Conditions
-------------------

The boundary condition for all boundaries is the "zero-flux" Neumann boundary condition. This is implemented in the spatial discretization by setting the concentration in the neighbouring pixel that lies outside the compartment boundary to be equal to the concentration in the boundary pixel value, or equivalently by setting the neighbour of each boundary pixel to itself.

Compartments
^^^^^^^^^^^^

Each compartment is discretized, with the above boundary conditions applied for the diffusion term.

Membranes
^^^^^^^^^

Reactions that take place between two compartments involve a flux across the membrane separating the two compartments. For each neighbouring pair of pixels from the two compartments, whose common boundary constitutes the membrane, the flux term is converted into a reaction term that creates or destroys the appropriate amount of species concentration in each pixel.

Non-spatial species
^^^^^^^^^^^^^^^^^^^

A species can be 'non-spatial', which means that at each timestep, its time derivative is calculated as normal at each point in the compartment, but is then spatially averaged over the whole compartment. This can be used to approximate a species with a very high diffusion constant without requiring a correspondingly tiny timestep to maintain the stability of the solver.

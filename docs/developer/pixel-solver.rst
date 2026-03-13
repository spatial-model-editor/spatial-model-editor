Pixel
=====

This page describes the implementation of the Pixel backend in
``core/simulate/src/pixelsim.cpp``, ``core/simulate/src/pixelsim_impl.cpp``
and ``core/simulate/src/cudapixelsim.cpp``.

It intentionally avoids repeating the numerical derivations. For the PDE,
units and discretization details, see :doc:`../reference/pixel` and
:doc:`../reference/maths`.

CPU backend
-----------

State layout
^^^^^^^^^^^^

``PixelSim`` builds two runtime object lists:

* one ``SimCompartment`` for each simulated compartment
* one ``SimMembrane`` for each membrane that has reactions and touches at
  least one simulated compartment

Each ``SimCompartment`` stores flattened voxel-major arrays, so the state for
voxel ``ix`` and species ``is`` lives at ``ix * nSpecies + is`` in
``conc``, ``dcdt`` and the Runge-Kutta work buffers.

If the model reactions depend on time and/or on the spatial coordinates, the
extra variables are appended to every voxel state in this order:

* ``time`` if needed
* ``x``, ``y``, ``z`` if needed

That padding is created once in the constructor and then carried through the
same symbolic evaluation path as the real species.

Compartment reactions
^^^^^^^^^^^^^^^^^^^^^

The compartment-local reaction path is built in two steps:

1. ``ReacExpr`` constructs one right-hand-side expression per species from the
   model reactions in that compartment, plus any optional ``time``/``x``/``y``/
   ``z`` variables.
2. ``SimCompartment`` compiles those expressions into a ``common::Symbolic``
   object so that each voxel update becomes a direct ``sym.eval(...)`` call.

At runtime, ``PixelSim::calculateDcdt()`` evaluates each compartment in this
order:

1. ``evaluateReactionsAndDiffusion()`` or ``evaluateReactionsAndDiffusion_tbb()``
2. membrane contributions
3. ``spatiallyAverageDcdt()`` for non-spatial species
4. ``applyStorage()`` or ``applyStorage_tbb()``

Inside ``SimCompartment``, the reaction contribution is the first pass over the
voxel state. ``evaluateReactions(begin, end)`` overwrites the corresponding
slice of ``dcdt`` with the compiled reaction result for each voxel. The later
diffusion and membrane passes add onto the same ``dcdt`` entries.

The diffusion pass is separate because it uses the compartment topology
(``up_x()``, ``dn_x()`` and friends) rather than the symbolic reaction system.
The implementation has two variants:

* a fast uniform-diffusion stencil when every species has a uniform diffusion
  constant
* a face-averaged conservative stencil when diffusion varies per voxel

If cross-diffusion is enabled, ``SimCompartment`` adds two extra passes:

* evaluate the cross-diffusion coefficients from symbolic expressions
* apply the cross-diffusion stencil and update the explicit stability bound

Zero-storage species reuse the same evaluation path. Before every Runge-Kutta
stage, ``solveZeroStorageConstraints()`` repeatedly calls the compartment and
membrane evaluators, then relaxes only the zero-storage species until the
residual is below the configured error tolerances.

Membrane reactions
^^^^^^^^^^^^^^^^^^

``SimMembrane`` implements membrane reactions as per-face updates into the two
adjacent compartments.

During construction it:

* builds the variable list as ``[species in A][species in B][extra vars]``
* rescales the symbolic membrane reaction by ``volOverL3`` so the compiled
  expression returns
  ``delta concentration * voxel length in the flux direction``
* compiles one ``common::Symbolic`` evaluator for the membrane

At runtime, ``evaluateMembranePairRange(...)`` processes a range of membrane
face pairs:

1. gather the concentrations from voxel ``ixA`` in compartment A and voxel
   ``ixB`` in compartment B into a temporary input buffer
2. evaluate the compiled symbolic membrane expression
3. add the resulting source terms into the two compartment ``dcdt`` arrays

The final division by voxel length is done per face direction at runtime:

* ``XP`` and ``XM`` divide by the voxel width
* ``YP`` and ``YM`` divide by the voxel height
* ``ZP`` and ``ZM`` divide by the voxel depth

That is why the membrane evaluator iterates over six signed face-direction
batches instead of using one merged pair list.

CPU parallelism
^^^^^^^^^^^^^^^

The CPU implementation uses TBB inside compartments and inside one membrane
face-direction batch, but it does not currently parallelize every outer loop.

Parallel today:

* ``SimCompartment::evaluateReactionsAndDiffusion_tbb()`` parallelizes the
  voxel loops for reaction evaluation, diffusion, cross-diffusion coefficient
  evaluation and cross-diffusion application.
* ``applyStorage_tbb()``, the Runge-Kutta update helpers and concentration
  clamping are parallelized over voxel ranges.
* ``SimMembrane::evaluateReactions_tbb()`` parallelizes the pair loop for one
  signed face direction at a time.

Serial today:

* ``PixelSim::calculateDcdt()`` iterates over compartments one after another.
* membranes are evaluated one membrane after another
* each membrane still processes ``XP``, ``XM``, ``YP``, ``YM``, ``ZP`` and
  ``ZM`` sequentially
* ``updateCrossDiffusionMaxStableTimestep()`` is a serial reduction over the
  already-evaluated coefficient field

The important race-avoidance rule is that the CPU membrane path writes directly
into the shared compartment ``dcdt`` buffers without atomics. Parallelism is
therefore limited to batches where each worker writes to disjoint output rows.
One signed face-direction batch has that property because a voxel can only have
one neighbour in a given signed Cartesian direction. Different face directions
do not have that guarantee, so they stay serial.

CUDA backend
------------

Current scope of the CUDA backend
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``CudaPixelSim`` is currently a proof-of-concept backend with stricter input
requirements than the CPU implementation. Setup fails early if the selected
model uses any of the following:

* cross-diffusion
* time-dependent reaction terms
* spatially dependent reaction terms
* non-spatial species
* non-uniform diffusion constants
* storage values other than ``1``

That reduced feature set is why the CUDA code path can use a simpler state
layout with no extra ``time`` or ``x``/``y``/``z`` padding.

Compartment kernels
^^^^^^^^^^^^^^^^^^^

For each simulated compartment, ``CudaPixelSim`` allocates:

* host buffers for concentrations, ``dcdt`` and lower-order RK data
* device buffers ``dConc``, ``dDcdt``, ``dLowerOrder`` and ``dOldConc``
* device buffers for neighbour indices and uniform diffusion coefficients
* one non-blocking CUDA stream and one ``dcdtReady`` event

The symbolic reaction expressions are converted to CUDA source with
``SymEngine::cudacode(...)`` and compiled at runtime with NVRTC.

The generated compartment kernels follow the same split as the CPU path:

* ``reaction_kernel`` writes the per-voxel reaction RHS into ``dDcdt``
* ``diffusion_uniform_kernel`` adds the uniform-diffusion stencil to
  ``dDcdt``

Membrane kernels
^^^^^^^^^^^^^^^^

Each membrane also gets its own generated CUDA kernel. The setup step uploads
one device array of voxel index pairs for every non-empty face direction and
stores the inverse face length for that batch.

The generated ``membrane_reaction_kernel``:

1. reads one face pair ``(ixA, ixB)``
2. gathers the species values from ``dConc`` in the two adjacent compartments
3. evaluates the generated membrane reaction function
4. uses ``atomicAdd`` to accumulate the result into ``dDcdtA`` and ``dDcdtB``

Because it uses atomics, the CUDA backend can run membrane batches that touch
the same compartment in parallel across different streams.

The membrane launches are pre-assigned round-robin across the compartment
streams. For each stream, ``membraneStreamDeps`` stores the compartments whose
``dcdtReady`` events must be satisfied before that stream can start its
assigned membrane kernels.

CUDA step flow
^^^^^^^^^^^^^^

The CUDA step flow tries to avoid synchronisation / waiting for kernels or transfers
wherever possible. In particular, all compartment kernels run in parallel as they
are independent of each other. The membrane kernels also run in parallel,
with atomic updates since multiple kernels can modify the same voxel.
Data transfer is non-blocking, so the next solver step can start while the
results of the previous step are still transferring to the host.

Additional details:

* ``evaluateDcdt()`` starts by recording ``mainStreamReady`` on the main
  compute stream. Each compartment stream waits on that event before launching
  its own reaction and diffusion kernels.
* After the compartment kernels finish, each stream records ``dcdtReady``.
  Streams assigned membrane work then wait only on the specific compartment
  events that their membrane batches touch.
* Once all compartment streams have recorded their completion events, the main
  stream waits on those events and then launches the Runge-Kutta update,
  clamping and error-estimation kernels.
* ``downloadStateToHost()`` records a ``computeDone`` event on the main stream,
  makes the dedicated transfer stream wait for it, then issues
  ``cuMemcpyDtoHAsync(...)`` copies for each compartment and records
  ``downloadComplete``.
* Future compute work calls ``ensureDownloadComplete()`` before writing to
  ``dConc`` again, so an in-flight download cannot race with the next kernel.
* Rejected adaptive steps restore ``dOldConc`` into ``dConc`` with
  ``cuMemcpyDtoDAsync(...)`` on the main compute stream.

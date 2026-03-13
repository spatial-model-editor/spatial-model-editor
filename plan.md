CUDA Pixel Backend PoC Plan
===========================

Goal
----
Add an optional CUDA execution backend for the existing pixel solver that
supports:

- compartment-local reaction terms
- diffusion
- RK101 only
- no membranes
- no cross-diffusion
- no zero-storage species
- no non-spatial species averaging
- no time/space-dependent reaction terms in the first cut

High-Level Design
-----------------
1. Keep `SimulatorType::Pixel` as-is and add a pixel execution backend option.
2. Leave the current CPU/TBB `PixelSim` implementation unchanged.
3. Add a new `CudaPixelSim` implementing `BaseSim`.
4. Select `PixelSim` vs `CudaPixelSim` in `Simulation`.
5. Share model/geometry analysis where practical, but keep CUDA runtime and
   kernel plumbing separate from the CPU implementation.

Backend Shape
-------------
1. Add `CudaPixelSim` in `core/simulate/src/cudapixelsim.hpp/.cpp`.
2. Add a small CUDA runtime wrapper for device/context/module handling.
3. Add a kernel source builder that wraps SymEngine-generated CUDA code in full
   kernel source.
4. Add a compiled-kernel wrapper around NVRTC + CUDA Driver API.
5. Add per-compartment CUDA state owning device buffers and cached kernel
   handles.

Data Layout
-----------
1. Keep voxel-major AoS layout for the PoC.
2. Store, per compartment:
   - concentrations
   - dcdt
   - neighbor index arrays
   - uniform diffusion constants or per-voxel diffusion arrays
3. Keep host mirrors for concentrations and dcdt so the existing `BaseSim`
   interface remains unchanged.

Code Generation
---------------
1. Assume SymEngine can emit a CUDA-compatible reaction operator string.
2. Treat that generated code as a device function, not a full kernel.
3. Wrap it in generated CUDA source containing:
   - `reaction_kernel`
   - `diffusion_uniform_kernel`
   - `diffusion_variable_kernel`
   - `rk101_update_kernel`
   - `clamp_negative_kernel`
4. Use fixed `extern "C"` kernel names for simple module lookup.
5. Cache compiled modules by reaction source, species count, diffusion mode,
   scalar type, and GPU architecture.

NVRTC / Driver API Flow
-----------------------
1. Detect device compute capability.
2. Generate CUDA source for a compartment.
3. Compile with NVRTC for the detected `sm_xy`.
4. Prefer loading CUBIN via the Driver API when available.
5. Resolve kernels with `cuModuleGetFunction`.
6. Launch kernels with `cuLaunchKernel`.

Kernel Structure
----------------
1. `reaction_kernel`: one thread per voxel, write reaction contribution into
   `dcdt`.
2. `diffusion_uniform_kernel`: one thread per voxel, gather 6 neighbors using
   precomputed compartment topology and add diffusion into `dcdt`.
3. `diffusion_variable_kernel`: same as above using per-voxel diffusion values.
4. `rk101_update_kernel`: update `conc += dt * dcdt`.
5. `clamp_negative_kernel`: clamp primary species after the step.

Run Loop
--------
1. Only support `PixelIntegratorType::RK101` in the PoC.
2. Reuse the CPU stable-timestep formulas on the host.
3. For each internal timestep and each compartment:
   - launch `reaction_kernel`
   - launch the appropriate diffusion kernel
   - launch `rk101_update_kernel`
   - launch `clamp_negative_kernel`
4. Synchronize once per outer `run()` call, then download results.

PoC Validation Rules
--------------------
Fail fast in `CudaPixelSim` if any of these are present:

- membrane reactions
- cross-diffusion
- zero-storage species
- non-spatial species
- adaptive integrators
- time-dependent reactions
- space-dependent reactions

Minimal Refactors
-----------------
1. Extract shared pixel-backend model validation from `PixelSim` where useful.
2. Reuse `ReacExpr` so CPU and CUDA backends build reaction bundles the same
   way.
3. Reuse existing `Compartment` neighbor topology rather than rebuilding it.

Build Plan
----------
1. Add a CMake option such as `SME_ENABLE_CUDA_PIXEL`.
2. Keep the project languages as `C` and `CXX`; kernels are JIT-compiled at
   runtime.
3. Use `find_package(CUDAToolkit)` for host-side headers and libraries.
4. Link the CUDA backend host code to NVRTC and the CUDA Driver API.

Testing Plan
------------
1. Add constructor/validation tests for unsupported features.
2. Add correctness tests against CPU `PixelSim` for:
   - reaction-only (`ABtoC`)
   - diffusion-only (`SingleCompartmentDiffusion`)
3. If GPU CI is unavailable, split source-generation tests from GPU integration
   tests.

Milestones
----------
1. Build integration, backend selection, and CUDA backend skeleton.
2. Hard-coded reaction-only kernel.
3. Generated reaction kernel from SymEngine CUDA printer.
4. Uniform diffusion kernel.
5. Variable diffusion kernel and restart support.

Deferred Work
-------------
- membranes
- cross-diffusion
- zero-storage relaxation
- adaptive Runge-Kutta
- time/space-dependent reactions
- data-layout tuning beyond AoS

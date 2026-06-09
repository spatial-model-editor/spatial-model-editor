# Plan: Show calculated feature in optimizer add-target dialog

Branch: `improve-optimizer-gui-for-features`

## Context

When adding an optimization target of type `Feature` in `DialogOptCost`, the user
selects a feature (defined by an ROI + reduction operation) and provides a **target
concentration image** (the same kind of image used for a `Concentration` target,
edited via "Edit Concentration"). The optimizer then calculates the target feature
values by applying the feature definition to that image, and minimises the difference
between those target values and the values extracted from the simulated concentration
at each optimization step.

The dialog currently shows the geometric ROI of the selected feature (which voxels
belong to which region, colour-coded by region index) in the `featureDisplay` panel
(`lblImageFeature`). The goal is to instead show the **calculated feature values**
— i.e., the per-region scalars that result from applying the feature to the current
target concentration image — as a greyscale image where every voxel in a region gets
the intensity corresponding to that region's calculated scalar value.

## What already exists

- `updateImageFeature()` in [gui/dialogs/dialogoptcost.cpp](gui/dialogs/dialogoptcost.cpp)
  currently paints the indexed-colour region map (placeholder).
- `sme::simulate::evaluateFeature()` in
  [core/simulate/include/sme/feature_eval.hpp](core/simulate/include/sme/feature_eval.hpp)
  takes per-voxel concentrations + pre-computed voxel regions and returns one `double`
  per ROI region.
- `sme::common::ImageStack(Volume, vector<double>, maxValue)` in
  [core/common/src/image_stack.cpp](core/common/src/image_stack.cpp) converts a
  full-geometry flat array of doubles to a normalised greyscale image. Already used
  for the optimizer diff image in [core/simulate/src/optimize.cpp](core/simulate/src/optimize.cpp#L406).
- The voxel→geometry index mapping pattern (used by `optimize.cpp:127-131`):
  ```cpp
  const auto imageIndex = common::voxelArrayIndex(imageSize, voxels[i], true);
  targetConcentrations[i] = optCost.targetValues[imageIndex];
  ```

## Implementation plan for `updateImageFeature()`

Replace the current placeholder region-colour painting with:

1. **Guard:** if `targetValues` is empty (user hasn't set a target image yet), clear
   `lblImageFeature` and return early.

2. **Extract per-voxel concentrations** from `m_optCost.targetValues`:
   - Get the compartment via `m_model.getCompartments().getCompartment(...)`.
   - For each compartment voxel, compute `imageIndex = common::voxelArrayIndex(imageSize, voxels[i], true)` and read `targetValues[imageIndex]`.
   - Result: `vector<double> targetConcentrations` of size `comp->nVoxels()`.

3. **Calculate feature values** per region:
   ```cpp
   auto regionValues = sme::simulate::evaluateFeature(
       feature, targetConcentrations, voxelRegions);
   // regionValues[r] is the scalar for region r+1 (0-indexed)
   ```

4. **Build a full-geometry scalar image** (size = `vol.nVoxels()`), initialised to 0:
   - For each compartment voxel `i`, look up its region from `voxelRegions[i]`
     (0 = excluded), then write `regionValues[region - 1]` into the full-geometry
     array at `common::voxelArrayIndex(imageSize, voxels[i], true)`.

5. **Render** via `ImageStack(vol, perVoxelValues)` → `lblImageFeature->setImage(...)`.

## Trigger

`updateImageFeature()` must be called:
- When the feature cost type is selected (`cmbCostType_currentIndexChanged`) — already done.
- After the user edits the target image (`btnEditTargetValues_clicked`) — **needs to be added**.
- When the species changes (`cmbSpecies_currentIndexChanged`) — already called.

## Notes

- No new colourmap needed. Greyscale is consistent with how concentration images are
  displayed elsewhere in the app. Magnitude is not encoded by colour in concentration
  views either.
- The geometric ROI structure is still implied: all voxels in the same region get the
  same greyscale intensity, so the region boundaries remain visible.
- If all region values are zero (e.g. target image is all-zero), the image will be
  black — same behaviour as an empty concentration image.

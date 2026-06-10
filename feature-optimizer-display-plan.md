# Plan: Show calculated feature images in the optimizer run dialog

Branch: `improve-optimizer-gui-for-features`

## Design note — are feature targets useful standalone?

A Feature cost is a reduction of the spatial concentration field into a scalar per
ROI region. It carries no spatial information on its own — the optimizer only sees
whether the per-region average/min/max etc. matches the target, not where
concentrations are spatially wrong. A feature target is therefore **degenerate
without a corresponding concentration target**: the optimizer could satisfy the
feature cost with wildly wrong spatial distributions that happen to have the right
regional aggregate.

This means the visualisation below is primarily useful for *diagnostic* purposes
(seeing how the feature value evolves during a run that also includes concentration
targets), not as a standalone optimisation view. Keep this in mind when deciding
whether to expose a separate feature display at all, or whether it is enough to
annotate the existing concentration images.

---

## Data flow background

`bestResults.values[index]` (populated by `calculateCosts` →
`setBestResults` inside `optimize_impl.cpp`) is the **full-geometry concentration
array** for the selected species — same size and indexing as `targetValues`.

`FeatureOptCost` (built once at `Optimization` construction by
`resolveFeatureOptCost`) already caches everything needed to convert that
concentration array into a feature image:

| Field | Content |
|---|---|
| `targetFeatureValues` | per-region scalars from `targetValues` (static) |
| `voxelRegions` | per-compartment-voxel region index (static) |
| `imageIndices` | precomputed flat geometry index for each compartment voxel (static) |
| `feature` | the `FeatureDefinition` (reduction op, ROI, etc.) |

Computing a feature image from the current best result is therefore:
1. Extract per-voxel concentrations: `voxelConcs[j] = bestResults.values[index][imageIndices[j]]`
2. `evaluateFeature(feature, voxelConcs, voxelRegions)` → per-region scalars
3. Scatter back: `perVoxelValues[imageIndices[j]] = regionScalars[voxelRegions[j]-1]`
4. `ImageStack(imageSize, perVoxelValues)` → greyscale image

---

## Layer 1 — `Optimization` class (core)

Three new methods in
[core/simulate/include/sme/optimize.hpp](core/simulate/include/sme/optimize.hpp)
and [core/simulate/src/optimize.cpp](core/simulate/src/optimize.cpp):

### `bool isFeatureTarget(std::size_t index) const`
Checks `optConstData->optimizeOptions.optCosts[index].optCostType ==
OptCostType::Feature`. `optConstData` is immutable after construction — no lock
needed.

### `ImageStack getFeatureTargetImage(std::size_t index) const`
Static — `targetFeatureValues` is precomputed. Scatter:
```
for j in 0..imageIndices.size()-1:
    if voxelRegions[j] != 0:
        perVoxelValues[imageIndices[j]] = targetFeatureValues[voxelRegions[j] - 1]
return ImageStack(imageSize, perVoxelValues)
```
No lock needed.

### `std::optional<ImageStack> getUpdatedBestFeatureResultImage(std::size_t index)`
Mirrors `getUpdatedBestResultImage` — same `bestResultsMutex` guard and
`imageChanged`/`imageIndex` check. When it should return a new image:
1. Lock `bestResultsMutex`, copy `bestResults.values[index]`
2. Extract per-voxel concentrations via `imageIndices`
3. `evaluateFeature(feature, voxelConcs, voxelRegions)` → per-region scalars
4. Scatter back, return `ImageStack(imageSize, perVoxelValues)`

---

## Layer 2 — `DialogOptimize` GUI

No new UI widgets. Existing `lblTarget`/`lblResult` (and their 3D variants) get
different content depending on target type. The `cmbTarget` combo already lists all
cost targets including Feature ones by name — no change needed there.

### New private helper in [dialogoptimize.hpp](gui/dialogs/dialogoptimize.hpp)
```cpp
bool selectedTargetIsFeature() const;
```
Returns `m_opt != nullptr && m_opt->isFeatureTarget(ui->cmbTarget->currentIndex())`.

### Changes in [dialogoptimize.cpp](gui/dialogs/dialogoptimize.cpp)

**`updateTargetImage()`** — branch on `selectedTargetIsFeature()`:
- Feature → `getFeatureTargetImage(index)`
- Concentration/Dcdt → `getTargetImage(index)` (unchanged)

**`updateResultImage()`** — branch on `selectedTargetIsFeature()`:
- Feature → `getUpdatedBestFeatureResultImage(index)` (fall through if empty)
- Concentration/Dcdt → unchanged

**`updatePlots()`** — the live-update call on each timer tick:
- Feature → `getUpdatedBestFeatureResultImage(index)`
- Concentration/Dcdt → `getUpdatedBestResultImage(index)` (unchanged)

`cmbTarget_currentIndexChanged()` already delegates to `updateTargetImage()` and
`updateResultImage()` — picks up the branching for free.

---

## Differences tab

`getDifferenceImage` does pixel-wise concentration subtraction. For Feature targets
this still shows where the simulated concentration differs from the target
concentration (the *input* to the feature), which is at least informative.
A true feature-level difference image (per-region scalar diff scattered back to
geometry) could be a later addition via a `getFeatureDifferenceImage` method.

---

## Touch points summary

| File | What changes |
|---|---|
| [core/simulate/include/sme/optimize.hpp](core/simulate/include/sme/optimize.hpp) | +3 method declarations |
| [core/simulate/src/optimize.cpp](core/simulate/src/optimize.cpp) | +3 method implementations |
| [gui/dialogs/dialogoptimize.hpp](gui/dialogs/dialogoptimize.hpp) | +1 helper declaration |
| [gui/dialogs/dialogoptimize.cpp](gui/dialogs/dialogoptimize.cpp) | +1 helper, branch in `updateTargetImage`, `updateResultImage`, `updatePlots` |
| [gui/dialogs/dialogoptimize.ui](gui/dialogs/dialogoptimize.ui) | no changes |

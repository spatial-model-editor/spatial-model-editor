#pragma once

#include "sme/feature_eval.hpp"
#include "sme/feature_options.hpp"
#include <cstddef>
#include <string>
#include <vector>

namespace sme::simulate {
class SimulationData;
}

namespace sme::model {

class ModelCompartments;
class ModelSpecies;
class ModelGeometry;
struct Settings;

/**
 * @brief Manager for feature extraction definitions and results.
 */
class ModelFeatures {
private:
  Settings *settings{nullptr};
  const ModelCompartments *modelCompartments{nullptr};
  const ModelSpecies *modelSpecies{nullptr};
  const ModelGeometry *modelGeometry{nullptr};
  simulate::SimulationData *simulationData{nullptr};
  std::vector<std::vector<std::size_t>> cachedVoxelRegions;
  bool hasUnsavedChanges{false};

  void recomputeVoxelRegions(std::size_t featureIndex);
  void ensureFeatureIdsValid();

public:
  ModelFeatures();
  ModelFeatures(Settings *settings, const ModelCompartments *compartments,
                const ModelSpecies *species, const ModelGeometry *geometry,
                simulate::SimulationData *simData);

  /**
   * @brief Add a new feature definition.
   * @returns Index of the newly added feature.
   */
  std::size_t add(const std::string &name, const std::string &compartmentId,
                  const std::string &speciesId,
                  const simulate::RoiSettings &roi,
                  simulate::ReductionOp reduction);

  /**
   * @brief Remove a feature by index.
   */
  void remove(std::size_t featureIndex);

  /**
   * @brief Set feature name.
   * @returns Unique feature name that was set.
   */
  std::string setName(std::size_t featureIndex, const std::string &name);

  /**
   * @brief Set feature compartment and species IDs.
   */
  void setCompartmentAndSpecies(std::size_t featureIndex,
                                const std::string &compartmentId,
                                const std::string &speciesId);

  /**
   * @brief Set feature ROI settings.
   */
  void setRoi(std::size_t featureIndex, const simulate::RoiSettings &roi);

  /**
   * @brief Set feature reduction operation.
   */
  void setReduction(std::size_t featureIndex, simulate::ReductionOp reduction);

  /**
   * @brief Get all feature definitions.
   */
  [[nodiscard]] const std::vector<simulate::FeatureDefinition> &
  getFeatures() const;

  /**
   * @brief Number of defined features.
   */
  [[nodiscard]] std::size_t size() const;

  /**
   * @brief Number of regions for a feature.
   */
  [[nodiscard]] std::size_t nRegions(std::size_t featureIndex) const;

  /**
   * @brief Get cached ROI regions for a feature.
   */
  [[nodiscard]] const std::vector<std::size_t> &
  getVoxelRegions(std::size_t featureIndex) const;

  /**
   * @brief Check if feature references valid compartment/species IDs.
   */
  [[nodiscard]] bool isValid(std::size_t featureIndex) const;

  /**
   * @brief Look up feature index from stable internal id.
   *
   * Returns ``size()`` if the id is not present.
   */
  [[nodiscard]] std::size_t getIndexFromId(const std::string &featureId) const;

  /**
   * @brief Evaluate all features at a given timepoint.
   *
   * Extracts species concentrations from SimulationData for the given
   * timeIndex, applies ROI regions and reduction, and appends results.
   */
  void evaluateAtTimepoint(std::size_t timeIndex);

  /**
   * @brief Re-evaluate all features over all stored timepoints.
   */
  void reEvaluateAllTimepoints();

  /**
   * @brief Recompute ROI regions after geometry change.
   */
  void updateGeometry();

  /**
   * @brief Update the simulation data pointer (after SME file import).
   */
  void setSimulationDataPtr(simulate::SimulationData *simData);

  void setHasUnsavedChanges(bool unsavedChanges);
  [[nodiscard]] bool getHasUnsavedChanges() const;
};

} // namespace sme::model

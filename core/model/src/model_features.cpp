#include "sme/model_features.hpp"
#include "id.hpp"
#include "sme/logger.hpp"
#include "sme/model_compartments.hpp"
#include "sme/model_geometry.hpp"
#include "sme/model_settings.hpp"
#include "sme/model_species.hpp"
#include "sme/simulate_data.hpp"
#include "sme/utils.hpp"
#include <algorithm>
#include <optional>

namespace sme::model {

static std::string
makeUniqueFeatureName(const std::string &name,
                      const std::vector<simulate::FeatureDefinition> &features,
                      std::optional<std::size_t> excludedIndex = std::nullopt) {
  QStringList names;
  for (std::size_t i = 0; i < features.size(); ++i) {
    if (!excludedIndex.has_value() || i != *excludedIndex) {
      names.push_back(QString::fromStdString(features[i].name));
    }
  }
  return makeUnique(QString::fromStdString(name), names).toStdString();
}

ModelFeatures::ModelFeatures() = default;

ModelFeatures::ModelFeatures(Settings *settings,
                             const ModelCompartments *compartments,
                             const ModelSpecies *species,
                             const ModelGeometry *geometry,
                             simulate::SimulationData *simData)
    : settings(settings), modelCompartments(compartments),
      modelSpecies(species), modelGeometry(geometry), simulationData(simData) {
  ensureFeatureIdsValid();
  cachedVoxelRegions.resize(settings->features.size());
  for (std::size_t i = 0; i < settings->features.size(); ++i) {
    if (isValid(i)) {
      recomputeVoxelRegions(i);
    }
  }
}

void ModelFeatures::ensureFeatureIdsValid() {
  if (settings == nullptr) {
    return;
  }
  std::vector<simulate::FeatureDefinition> existingFeatures;
  existingFeatures.reserve(settings->features.size());
  for (auto &feature : settings->features) {
    feature.name = makeUniqueFeatureName(feature.name, existingFeatures);
    if (feature.id.empty() ||
        std::ranges::any_of(existingFeatures, [&feature](const auto &existing) {
          return existing.id == feature.id;
        })) {
      feature.id =
          simulate::makeUniqueFeatureId(feature.name, existingFeatures);
    }
    existingFeatures.push_back(feature);
  }
}

void ModelFeatures::recomputeVoxelRegions(std::size_t featureIndex) {
  if (settings == nullptr || modelCompartments == nullptr ||
      modelGeometry == nullptr) {
    return;
  }
  const auto &feat = settings->features[featureIndex];
  auto *comp = modelCompartments->getCompartment(feat.compartmentId.c_str());
  if (comp == nullptr) {
    cachedVoxelRegions[featureIndex].clear();
    return;
  }
  cachedVoxelRegions[featureIndex] = simulate::computeVoxelRegions(
      feat.roi, *comp, modelGeometry->getPhysicalOrigin(),
      modelGeometry->getVoxelSize());
}

std::size_t ModelFeatures::add(const std::string &name,
                               const std::string &compartmentId,
                               const std::string &speciesId,
                               const simulate::RoiSettings &roi,
                               simulate::ReductionOp reduction) {
  simulate::FeatureDefinition feat;
  feat.name = makeUniqueFeatureName(name, settings->features);
  feat.id = simulate::makeUniqueFeatureId(feat.name, settings->features);
  feat.compartmentId = compartmentId;
  feat.speciesId = speciesId;
  feat.roi = roi;
  feat.reduction = reduction;
  settings->features.push_back(std::move(feat));
  auto index = settings->features.size() - 1;
  cachedVoxelRegions.emplace_back();
  if (simulationData != nullptr) {
    simulationData->featureResults.emplace_back();
  }
  if (isValid(index)) {
    recomputeVoxelRegions(index);
  }
  hasUnsavedChanges = true;
  return index;
}

void ModelFeatures::remove(std::size_t featureIndex) {
  if (featureIndex >= settings->features.size()) {
    return;
  }
  settings->features.erase(settings->features.begin() +
                           static_cast<std::ptrdiff_t>(featureIndex));
  cachedVoxelRegions.erase(cachedVoxelRegions.begin() +
                           static_cast<std::ptrdiff_t>(featureIndex));
  if (simulationData != nullptr &&
      featureIndex < simulationData->featureResults.size()) {
    simulationData->featureResults.erase(
        simulationData->featureResults.begin() +
        static_cast<std::ptrdiff_t>(featureIndex));
  }
  hasUnsavedChanges = true;
}

std::string ModelFeatures::setName(std::size_t featureIndex,
                                   const std::string &name) {
  if (featureIndex < settings->features.size()) {
    settings->features[featureIndex].name =
        makeUniqueFeatureName(name, settings->features, featureIndex);
    hasUnsavedChanges = true;
    return settings->features[featureIndex].name;
  }
  return {};
}

void ModelFeatures::setCompartmentAndSpecies(std::size_t featureIndex,
                                             const std::string &compartmentId,
                                             const std::string &speciesId) {
  if (featureIndex >= settings->features.size()) {
    return;
  }
  auto &feat = settings->features[featureIndex];
  feat.compartmentId = compartmentId;
  feat.speciesId = speciesId;
  recomputeVoxelRegions(featureIndex);
  hasUnsavedChanges = true;
}

void ModelFeatures::setRoi(std::size_t featureIndex,
                           const simulate::RoiSettings &roi) {
  if (featureIndex >= settings->features.size()) {
    return;
  }
  settings->features[featureIndex].roi = roi;
  if (isValid(featureIndex)) {
    recomputeVoxelRegions(featureIndex);
  }
  hasUnsavedChanges = true;
}

void ModelFeatures::setReduction(std::size_t featureIndex,
                                 simulate::ReductionOp reduction) {
  if (featureIndex >= settings->features.size()) {
    return;
  }
  settings->features[featureIndex].reduction = reduction;
  hasUnsavedChanges = true;
}

const std::vector<simulate::FeatureDefinition> &
ModelFeatures::getFeatures() const {
  static const std::vector<simulate::FeatureDefinition> empty;
  if (settings == nullptr) {
    return empty;
  }
  return settings->features;
}

std::size_t ModelFeatures::size() const {
  if (settings == nullptr) {
    return 0;
  }
  return settings->features.size();
}

std::size_t ModelFeatures::nRegions(std::size_t featureIndex) const {
  if (featureIndex >= settings->features.size()) {
    return 0;
  }
  return simulate::getNumRegions(settings->features[featureIndex].roi);
}

const std::vector<std::size_t> &
ModelFeatures::getVoxelRegions(std::size_t featureIndex) const {
  return cachedVoxelRegions[featureIndex];
}

bool ModelFeatures::isValid(std::size_t featureIndex) const {
  if (settings == nullptr || modelCompartments == nullptr ||
      modelSpecies == nullptr) {
    return false;
  }
  if (featureIndex >= settings->features.size()) {
    return false;
  }
  const auto &feat = settings->features[featureIndex];
  const auto &compIds = modelCompartments->getIds();
  if (!compIds.contains(feat.compartmentId.c_str())) {
    return false;
  }
  const auto specIds = modelSpecies->getIds(feat.compartmentId.c_str());
  return specIds.contains(feat.speciesId.c_str());
}

std::size_t ModelFeatures::getIndexFromId(const std::string &featureId) const {
  if (settings == nullptr) {
    return 0;
  }
  for (std::size_t i = 0; i < settings->features.size(); ++i) {
    if (settings->features[i].id == featureId) {
      return i;
    }
  }
  return settings->features.size();
}

// Cached index info for a feature, computed once and reused across timepoints
struct FeatureIndices {
  std::size_t simCompIndex{0};
  std::size_t speciesIndex{0};
  std::size_t nSimSpecies{0};
  std::size_t nVoxels{0};
  bool valid{false};
};

static FeatureIndices
computeFeatureIndices(const simulate::FeatureDefinition &feat,
                      const ModelCompartments &compartments,
                      const ModelSpecies &species) {
  FeatureIndices idx;
  const auto &compIds = compartments.getIds();
  auto compPos = compIds.indexOf(feat.compartmentId.c_str());
  if (compPos < 0) {
    return idx;
  }
  auto specIds = species.getIds(feat.compartmentId.c_str());
  for (int i = 0; i < specIds.size(); ++i) {
    if (species.isSimulatedSpecies(specIds[i])) {
      if (specIds[i] == feat.speciesId.c_str()) {
        idx.speciesIndex = idx.nSimSpecies;
      }
      ++idx.nSimSpecies;
    }
  }
  if (idx.nSimSpecies == 0) {
    return idx;
  }
  auto *comp = compartments.getCompartment(feat.compartmentId.c_str());
  if (comp == nullptr) {
    return idx;
  }
  idx.nVoxels = comp->nVoxels();
  // find the simulation compartment index (only compartments with simulated
  // species are included in concentration data)
  for (int i = 0; i < compIds.size(); ++i) {
    if (compIds[i] == feat.compartmentId.c_str()) {
      idx.valid = true;
      break;
    }
    auto sIds = species.getIds(compIds[i]);
    for (const auto &s : sIds) {
      if (species.isSimulatedSpecies(s)) {
        ++idx.simCompIndex;
        break;
      }
    }
  }
  return idx;
}

// Extract per-species concentration from interleaved SimulationData into buffer
static void extractSpeciesConc(std::vector<double> &concs,
                               const simulate::SimulationData &data,
                               std::size_t timeIndex,
                               std::size_t compartmentIndex,
                               std::size_t speciesIndex, std::size_t nVoxels,
                               std::size_t nSpecies) {
  const auto &compConc = data.concentration[timeIndex][compartmentIndex];
  std::size_t stride = nSpecies + data.concPadding[timeIndex];
  concs.resize(nVoxels);
  for (std::size_t ix = 0; ix < nVoxels; ++ix) {
    concs[ix] = compConc[ix * stride + speciesIndex];
  }
}

void ModelFeatures::evaluateAtTimepoint(std::size_t timeIndex) {
  if (settings == nullptr || simulationData == nullptr ||
      modelCompartments == nullptr || modelSpecies == nullptr) {
    return;
  }
  simulationData->featureResults.resize(settings->features.size());
  std::vector<double> concs; // reusable buffer
  for (std::size_t fi = 0; fi < settings->features.size(); ++fi) {
    if (!isValid(fi) || cachedVoxelRegions[fi].empty()) {
      simulationData->featureResults[fi].values.emplace_back();
      continue;
    }
    const auto &feat = settings->features[fi];
    auto idx = computeFeatureIndices(feat, *modelCompartments, *modelSpecies);
    if (!idx.valid ||
        idx.simCompIndex >= simulationData->concentration[timeIndex].size()) {
      simulationData->featureResults[fi].values.emplace_back();
      continue;
    }
    extractSpeciesConc(concs, *simulationData, timeIndex, idx.simCompIndex,
                       idx.speciesIndex, idx.nVoxels, idx.nSimSpecies);
    auto vals = simulate::evaluateFeature(feat, concs, cachedVoxelRegions[fi]);
    simulationData->featureResults[fi].values.push_back(std::move(vals));
  }
}

void ModelFeatures::reEvaluateAllTimepoints() {
  if (settings == nullptr || simulationData == nullptr) {
    return;
  }
  // clear existing results
  simulationData->featureResults.clear();
  simulationData->featureResults.resize(settings->features.size());
  // re-evaluate for each stored timepoint
  for (std::size_t t = 0; t < simulationData->timePoints.size(); ++t) {
    evaluateAtTimepoint(t);
  }
}

void ModelFeatures::updateGeometry() {
  if (settings == nullptr) {
    return;
  }
  cachedVoxelRegions.resize(settings->features.size());
  for (std::size_t i = 0; i < settings->features.size(); ++i) {
    if (isValid(i)) {
      recomputeVoxelRegions(i);
    } else {
      cachedVoxelRegions[i].clear();
    }
  }
}

void ModelFeatures::setSimulationDataPtr(simulate::SimulationData *simData) {
  simulationData = simData;
}

void ModelFeatures::setHasUnsavedChanges(bool unsavedChanges) {
  hasUnsavedChanges = unsavedChanges;
}

bool ModelFeatures::getHasUnsavedChanges() const { return hasUnsavedChanges; }

} // namespace sme::model

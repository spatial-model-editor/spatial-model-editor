#pragma once

#include <cereal/cereal.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <cstddef>
#include <map>
#include <string>
#include <vector>

namespace sme::simulate {

/**
 * @brief ROI source type for feature extraction.
 */
enum class RoiType { Analytic, Image, Depth, AxisSlices };

/**
 * @brief Reduction operation applied over voxels in a region.
 */
enum class ReductionOp { Average, Sum, Min, Max };

/**
 * @brief Generic named ROI parameter value.
 *
 * Boolean parameters are stored in intValue and exposed through typed helpers.
 */
struct RoiParameterValue {
  int intValue{0};
  double doubleValue{0.0};

  template <class Archive>
  void serialize(Archive &ar, std::uint32_t const version) {
    if (version == 0) {
      ar(CEREAL_NVP(intValue), CEREAL_NVP(doubleValue));
    }
  }
};

/**
 * @brief ROI settings defining how voxels are assigned to ROI regions.
 */
struct RoiSettings {
  /**
   * @brief Type of ROI source.
   */
  RoiType roiType{RoiType::Analytic};
  /**
   * @brief Analytic expression for regions (variables: x, y, z).
   *
   * The result is cast to ``int`` for each voxel:
   * 0 = excluded, 1..N = region index.
   */
  std::string expression{"1"};
  /**
   * @brief Number of ROI regions.
   */
  std::size_t numRegions{1};
  /**
   * @brief Imported image regions (pixel values = region assignments).
   *
   * 0 = excluded, 1..N = region index.
   */
  std::vector<std::size_t> regionImage;
  /**
   * @brief Named parameters for ROI sources.
   */
  std::map<std::string, RoiParameterValue> parameters;

  template <class Archive>
  void serialize(Archive &ar, std::uint32_t const version) {
    if (version == 0) {
      ar(CEREAL_NVP(roiType), CEREAL_NVP(expression), CEREAL_NVP(numRegions),
         CEREAL_NVP(regionImage), CEREAL_NVP(parameters));
    }
  }
};

/**
 * @brief Defines a feature to extract from simulation results.
 */
struct FeatureDefinition {
  /**
   * @brief Stable internal feature id.
   */
  std::string id;
  /**
   * @brief User-visible feature name.
   */
  std::string name;
  /**
   * @brief SBML compartment id (by id for robustness across edits).
   */
  std::string compartmentId;
  /**
   * @brief SBML species id (by id for robustness across edits).
   */
  std::string speciesId;
  /**
   * @brief ROI settings.
   */
  RoiSettings roi;
  /**
   * @brief Reduction operation to apply per region.
   */
  ReductionOp reduction{ReductionOp::Average};

  template <class Archive>
  void serialize(Archive &ar, std::uint32_t const version) {
    if (version == 0) {
      ar(CEREAL_NVP(id), CEREAL_NVP(name), CEREAL_NVP(compartmentId),
         CEREAL_NVP(speciesId), CEREAL_NVP(roi), CEREAL_NVP(reduction));
    }
  }
};

/**
 * @brief Result of feature evaluation over simulation timepoints.
 */
struct FeatureResult {
  /**
   * @brief Feature values: ``values[timeIndex][regionIndex]``.
   */
  std::vector<std::vector<double>> values;

  template <class Archive>
  void serialize(Archive &ar, std::uint32_t const version) {
    if (version == 0) {
      ar(CEREAL_NVP(values));
    }
  }
};

namespace roi_param {
inline constexpr auto axis = "axis";
inline constexpr auto depthThicknessVoxels = "thickness_voxels";
} // namespace roi_param

std::string toString(RoiType roiType);
std::string toString(ReductionOp reduction);
int getRoiParameterInt(const RoiSettings &roi, const std::string &key,
                       int defaultValue);
double getRoiParameterDouble(const RoiSettings &roi, const std::string &key,
                             double defaultValue);
bool getRoiParameterBool(const RoiSettings &roi, const std::string &key,
                         bool defaultValue);
void setRoiParameterInt(RoiSettings &roi, const std::string &key, int value);
void setRoiParameterDouble(RoiSettings &roi, const std::string &key,
                           double value);
void setRoiParameterBool(RoiSettings &roi, const std::string &key, bool value);
std::string
makeUniqueFeatureId(const std::string &name,
                    const std::vector<FeatureDefinition> &existingFeatures);

} // namespace sme::simulate

CEREAL_CLASS_VERSION(sme::simulate::RoiParameterValue, 0);
CEREAL_CLASS_VERSION(sme::simulate::RoiSettings, 0);
CEREAL_CLASS_VERSION(sme::simulate::FeatureDefinition, 0);
CEREAL_CLASS_VERSION(sme::simulate::FeatureResult, 0);

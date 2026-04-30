#include "sme/feature_options.hpp"
#include "sme/id.hpp"
#include <algorithm>

namespace sme::simulate {

std::string toString(RoiType roiType) {
  using enum RoiType;
  switch (roiType) {
  case Analytic:
    return "Analytic";
  case Image:
    return "Image";
  case Depth:
    return "Depth";
  case AxisSlices:
    return "Axis slices";
  default:
    return "";
  }
}

std::string toString(ReductionOp reduction) {
  using enum ReductionOp;
  switch (reduction) {
  case Average:
    return "Average";
  case Sum:
    return "Sum";
  case Min:
    return "Min";
  case Max:
    return "Max";
  case FirstQuartile:
    return "First quartile";
  case Median:
    return "Median";
  case ThirdQuartile:
    return "Third quartile";
  default:
    return "";
  }
}

int getRoiParameterInt(const RoiSettings &roi, const std::string &key,
                       int defaultValue) {
  auto it = roi.parameters.find(key);
  return it == roi.parameters.end() ? defaultValue : it->second.intValue;
}

double getRoiParameterDouble(const RoiSettings &roi, const std::string &key,
                             double defaultValue) {
  auto it = roi.parameters.find(key);
  return it == roi.parameters.end() ? defaultValue : it->second.doubleValue;
}

bool getRoiParameterBool(const RoiSettings &roi, const std::string &key,
                         bool defaultValue) {
  return getRoiParameterInt(roi, key, defaultValue ? 1 : 0) != 0;
}

void setRoiParameterInt(RoiSettings &roi, const std::string &key, int value) {
  roi.parameters[key].intValue = value;
}

void setRoiParameterDouble(RoiSettings &roi, const std::string &key,
                           double value) {
  roi.parameters[key].doubleValue = value;
}

void setRoiParameterBool(RoiSettings &roi, const std::string &key, bool value) {
  setRoiParameterInt(roi, key, value ? 1 : 0);
}

std::string
makeUniqueFeatureId(const std::string &name,
                    const std::vector<FeatureDefinition> &existingFeatures) {
  return sme::common::makeUnique(
      sme::common::nameToSId(name, "feature"),
      [&existingFeatures](const std::string &candidate) {
        return std::ranges::none_of(
            existingFeatures,
            [&candidate](const auto &f) { return f.id == candidate; });
      });
}

} // namespace sme::simulate

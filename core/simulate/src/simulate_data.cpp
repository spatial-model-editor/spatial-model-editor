#include "sme/simulate_data.hpp"
#include "sme/utils.hpp"
#include <limits>

namespace sme::simulate {

namespace {

[[nodiscard]] std::size_t saturatingMul(std::size_t a, std::size_t b) {
  if (a == 0 || b == 0) {
    return 0;
  }
  if (a > std::numeric_limits<std::size_t>::max() / b) {
    return std::numeric_limits<std::size_t>::max();
  }
  return a * b;
}

template <typename T>
[[nodiscard]] std::size_t
get2dElementsBytes(const std::vector<std::vector<T>> &vec) {
  std::size_t bytes{0};
  for (const auto &inner : vec) {
    bytes =
        common::saturatingAdd(bytes, saturatingMul(inner.size(), sizeof(T)));
  }
  return bytes;
}

template <typename T>
[[nodiscard]] std::size_t
get3dElementsBytes(const std::vector<std::vector<std::vector<T>>> &vec) {
  std::size_t bytes{0};
  for (const auto &middle : vec) {
    bytes = common::saturatingAdd(bytes, get2dElementsBytes(middle));
  }
  return bytes;
}

} // namespace

void SimulationData::clear() {
  timePoints.clear();
  concentration.clear();
  avgMinMax.clear();
  concentrationMax.clear();
  concPadding.clear();
  xmlModel.clear();
}

std::size_t SimulationData::size() const { return timePoints.size(); }

std::size_t SimulationData::getEstimatedMemoryBytes() const {
  std::size_t bytes{0};
  bytes = common::saturatingAdd(
      bytes, saturatingMul(timePoints.size(), sizeof(double)));
  bytes = common::saturatingAdd(
      bytes, saturatingMul(concPadding.size(), sizeof(std::size_t)));
  bytes = common::saturatingAdd(bytes, get3dElementsBytes(concentration));
  bytes = common::saturatingAdd(bytes, get3dElementsBytes(avgMinMax));
  bytes = common::saturatingAdd(bytes, get3dElementsBytes(concentrationMax));
  bytes = common::saturatingAdd(bytes,
                                saturatingMul(xmlModel.size(), sizeof(char)));
  return bytes;
}

std::size_t SimulationData::getEstimatedAdditionalMemoryBytes(
    std::size_t nAdditionalTimepoints) const {
  if (nAdditionalTimepoints == 0 || concentration.empty()) {
    return 0;
  }
  std::size_t bytesPerTimepoint{sizeof(double) + sizeof(std::size_t)};
  bytesPerTimepoint = common::saturatingAdd(
      bytesPerTimepoint, get2dElementsBytes(concentration.back()));
  if (!avgMinMax.empty()) {
    bytesPerTimepoint = common::saturatingAdd(
        bytesPerTimepoint, get2dElementsBytes(avgMinMax.back()));
  }
  if (!concentrationMax.empty()) {
    bytesPerTimepoint = common::saturatingAdd(
        bytesPerTimepoint, get2dElementsBytes(concentrationMax.back()));
  }
  return saturatingMul(bytesPerTimepoint, nAdditionalTimepoints);
}

void SimulationData::reserve(std::size_t n) {
  timePoints.reserve(n);
  concentration.reserve(n);
  avgMinMax.reserve(n);
  concentrationMax.reserve(n);
  concPadding.reserve(n);
}

void SimulationData::pop_back() {
  timePoints.pop_back();
  concentration.pop_back();
  avgMinMax.pop_back();
  concentrationMax.pop_back();
  concPadding.pop_back();
}

} // namespace sme::simulate

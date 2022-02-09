#include "sme/simulate_options.hpp"
#include <QStringList>
#include <cmath>

namespace sme::simulate {

std::optional<std::vector<std::pair<std::size_t, double>>>
parseSimulationTimes(const QString &lengths, const QString &intervals) {
  std::vector<std::pair<std::size_t, double>> times;
  auto simLengths{lengths.split(";")};
  auto simDtImages{intervals.split(";")};
  if (simLengths.empty() || simDtImages.empty()) {
    return {};
  }
  if (simLengths.size() != simDtImages.size()) {
    return {};
  }
  for (int i = 0; i < simLengths.size(); ++i) {
    bool validSimLengthDbl;
    double simLength{std::abs(simLengths[i].toDouble(&validSimLengthDbl))};
    bool validDtImageDbl;
    double dt{std::abs(simDtImages[i].toDouble(&validDtImageDbl))};
    if (!validDtImageDbl || !validSimLengthDbl) {
      return {};
    }
    int nImages{static_cast<int>(std::round(simLength / dt))};
    if (nImages < 1) {
      nImages = 1;
    }
    dt = simLength / static_cast<double>(nImages);
    times.emplace_back(nImages, dt);
  }
  return times;
}

bool operator==(const AvgMinMax &lhs, const AvgMinMax &rhs) {
  return (lhs.avg == rhs.avg) && (lhs.min == rhs.min) && (lhs.max == rhs.max);
}

} // namespace sme::simulate

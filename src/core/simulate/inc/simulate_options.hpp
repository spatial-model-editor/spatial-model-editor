// Simulator Options

#pragma once

#include <cstddef>
#include <limits>
#include <string>
#include <vector>
#include <utility>

namespace sme::simulate {

enum class SimulatorType { DUNE, Pixel };

enum class DuneDiscretizationType { FEM1 };

struct DuneOptions {
  DuneDiscretizationType discretization{DuneDiscretizationType::FEM1};
  std::string integrator{"alexander_2"};
  double dt{1e-1};
  double minDt{1e-10};
  double maxDt{1e4};
  double increase{1.5};
  double decrease{0.5};
  bool writeVTKfiles{false};
  double newtonRelErr{1e-8};
  double newtonAbsErr{1e-12};
};

enum class PixelIntegratorType { RK101, RK212, RK323, RK435 };

struct PixelIntegratorError {
  double abs{std::numeric_limits<double>::max()};
  double rel{0.005};
};

struct PixelOptions {
  PixelIntegratorType integrator{PixelIntegratorType::RK212};
  PixelIntegratorError maxErr;
  double maxTimestep{std::numeric_limits<double>::max()};
  bool enableMultiThreading{false};
  std::size_t maxThreads{0};
  bool doCSE{true};
  unsigned optLevel{3};
};

struct Options {
  DuneOptions dune;
  PixelOptions pixel;
};

struct SimulationSettings {
  std::vector<std::pair<std::size_t, double>> times;
  simulate::Options options;
  sme::simulate::SimulatorType simulatorType;
};

struct AvgMinMax {
  double avg = 0;
  double min = std::numeric_limits<double>::max();
  double max = 0;
};

bool operator==(const AvgMinMax &lhs, const AvgMinMax &rhs);

} // namespace sme::simulate

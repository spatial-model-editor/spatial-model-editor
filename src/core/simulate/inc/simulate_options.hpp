// Simulator Options

#pragma once

#include <cstddef>
#include <limits>

namespace simulate {

enum class DuneIntegratorType { FEM1 };

struct DuneOptions {
  DuneIntegratorType integrator{DuneIntegratorType::FEM1};
  double dt{0.01};
  bool writeVTKfiles{false};
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
  std::size_t maxThreads{0};
};

struct Options {
  DuneOptions dune;
  PixelOptions pixel;
};

} // namespace simulate

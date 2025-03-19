// Simulator Options

#pragma once

#include <QString>
#include <cereal/cereal.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/utility.hpp>
#include <cstddef>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace sme::simulate {

std::optional<std::vector<std::pair<std::size_t, double>>>
parseSimulationTimes(const QString &lengths, const QString &intervals);

enum class SimulatorType { DUNE, Pixel, DUNESteadyState, PixelSteadyState };

enum class DuneDiscretizationType { FEM1 };

struct DuneOptions {
  DuneDiscretizationType discretization{DuneDiscretizationType::FEM1};
  std::string integrator{"Alexander2"};
  double dt{1e-1};
  double minDt{1e-10};
  double maxDt{1e4};
  double increase{1.5};
  double decrease{0.5};
  bool writeVTKfiles{false};
  double newtonRelErr{1e-8};
  double newtonAbsErr{0.0};
  std::string linearSolver{"RestartedGMRes"};

  template <class Archive>
  void serialize(Archive &ar, std::uint32_t const version) {
    if (version == 0) {
      ar(CEREAL_NVP(discretization), CEREAL_NVP(integrator), CEREAL_NVP(dt),
         CEREAL_NVP(minDt), CEREAL_NVP(maxDt), CEREAL_NVP(increase),
         CEREAL_NVP(decrease), CEREAL_NVP(writeVTKfiles),
         CEREAL_NVP(newtonRelErr), CEREAL_NVP(newtonAbsErr));
    } else if (version == 1) {
      ar(CEREAL_NVP(discretization), CEREAL_NVP(integrator), CEREAL_NVP(dt),
         CEREAL_NVP(minDt), CEREAL_NVP(maxDt), CEREAL_NVP(increase),
         CEREAL_NVP(decrease), CEREAL_NVP(writeVTKfiles),
         CEREAL_NVP(newtonRelErr), CEREAL_NVP(newtonAbsErr),
         CEREAL_NVP(linearSolver));
    }
  }
};

enum class PixelIntegratorType { RK101, RK212, RK323, RK435 };

struct PixelIntegratorError {
  double abs{std::numeric_limits<double>::max()};
  double rel{0.005};

  template <class Archive>
  void serialize(Archive &ar, std::uint32_t const version) {
    if (version == 0) {
      ar(CEREAL_NVP(abs), CEREAL_NVP(rel));
    }
  }
};

struct PixelOptions {
  PixelIntegratorType integrator{PixelIntegratorType::RK212};
  PixelIntegratorError maxErr;
  double maxTimestep{std::numeric_limits<double>::max()};
  bool enableMultiThreading{false};
  std::size_t maxThreads{0};
  bool doCSE{true};
  unsigned optLevel{3};

  template <class Archive>
  void serialize(Archive &ar, std::uint32_t const version) {
    if (version == 0) {
      ar(CEREAL_NVP(integrator), CEREAL_NVP(maxErr), CEREAL_NVP(maxTimestep),
         CEREAL_NVP(enableMultiThreading), CEREAL_NVP(maxThreads),
         CEREAL_NVP(doCSE), CEREAL_NVP(optLevel));
    }
  }
};

struct Options {
  DuneOptions dune;
  PixelOptions pixel;

  template <class Archive>
  void serialize(Archive &ar, std::uint32_t const version) {
    if (version == 0) {
      ar(CEREAL_NVP(dune), CEREAL_NVP(pixel));
    }
  }
};

struct AvgMinMax {
  double avg = 0;
  double min = std::numeric_limits<double>::max();
  double max = 0;

  template <class Archive>
  void serialize(Archive &ar, std::uint32_t const version) {
    if (version == 0) {
      ar(avg, min, max);
    }
  }
  friend bool operator==(const AvgMinMax &lhs, const AvgMinMax &rhs);
};

} // namespace sme::simulate

CEREAL_CLASS_VERSION(sme::simulate::Options, 0);
CEREAL_CLASS_VERSION(sme::simulate::DuneOptions, 1);
CEREAL_CLASS_VERSION(sme::simulate::PixelIntegratorError, 0);
CEREAL_CLASS_VERSION(sme::simulate::PixelOptions, 0);
CEREAL_CLASS_VERSION(sme::simulate::AvgMinMax, 0);

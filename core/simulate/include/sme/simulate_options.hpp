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

/**
 * @brief Parse textual simulation segment lengths/intervals.
 */
std::optional<std::vector<std::pair<std::size_t, double>>>
parseSimulationTimes(const QString &lengths, const QString &intervals);

/**
 * @brief Solver backend selection.
 */
enum class SimulatorType { DUNE, Pixel };

/**
 * @brief DUNE spatial discretization type.
 */
enum class DuneDiscretizationType { FEM1 };

/**
 * @brief DUNE solver options.
 */
struct DuneOptions {
  /**
   * @brief Spatial discretization scheme.
   */
  DuneDiscretizationType discretization{DuneDiscretizationType::FEM1};
  /**
   * @brief Time integrator name.
   */
  std::string integrator{"Alexander2"};
  /**
   * @brief Initial timestep.
   */
  double dt{1e-1};
  /**
   * @brief Minimum adaptive timestep.
   */
  double minDt{1e-10};
  /**
   * @brief Maximum adaptive timestep.
   */
  double maxDt{1e4};
  /**
   * @brief Adaptive timestep increase factor.
   */
  double increase{1.5};
  /**
   * @brief Adaptive timestep decrease factor.
   */
  double decrease{0.5};
  /**
   * @brief Enable VTK output files.
   */
  bool writeVTKfiles{false};
  /**
   * @brief Newton relative tolerance.
   */
  double newtonRelErr{1e-8};
  /**
   * @brief Newton absolute tolerance.
   */
  double newtonAbsErr{0.0};
  /**
   * @brief Linear solver name.
   */
  std::string linearSolver{"RestartedGMRes"};
  /**
   * @brief Max thread count (0 means automatic/default).
   */
  std::size_t maxThreads{0};

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
    } else if (version == 2) {
      ar(CEREAL_NVP(discretization), CEREAL_NVP(integrator), CEREAL_NVP(dt),
         CEREAL_NVP(minDt), CEREAL_NVP(maxDt), CEREAL_NVP(increase),
         CEREAL_NVP(decrease), CEREAL_NVP(writeVTKfiles),
         CEREAL_NVP(newtonRelErr), CEREAL_NVP(newtonAbsErr),
         CEREAL_NVP(linearSolver), CEREAL_NVP(maxThreads));
    }
  }
};

/**
 * @brief Pixel integrator scheme selection.
 */
enum class PixelIntegratorType { RK101, RK212, RK323, RK435 };

/**
 * @brief Pixel execution backend selection.
 */
enum class PixelBackendType { CPU, GPU };

/**
 * @brief Floating-point precision for the GPU pixel backend.
 */
enum class GpuFloatPrecision { Double, Float };

/**
 * @brief Error tolerances for pixel adaptive integration.
 */
struct PixelIntegratorError {
  /**
   * @brief Absolute tolerance.
   */
  double abs{std::numeric_limits<double>::max()};
  /**
   * @brief Relative tolerance.
   */
  double rel{0.005};

  template <class Archive>
  void serialize(Archive &ar, std::uint32_t const version) {
    if (version == 0) {
      ar(CEREAL_NVP(abs), CEREAL_NVP(rel));
    }
  }
};

/**
 * @brief Pixel solver options.
 */
struct PixelOptions {
  /**
   * @brief Execution backend for the pixel solver.
   */
  PixelBackendType backend{PixelBackendType::CPU};
  /**
   * @brief Floating-point precision for the GPU backend.
   */
  GpuFloatPrecision gpuFloatPrecision{GpuFloatPrecision::Double};
  /**
   * @brief RK integrator scheme.
   */
  PixelIntegratorType integrator{PixelIntegratorType::RK212};
  /**
   * @brief Adaptive error tolerances.
   */
  PixelIntegratorError maxErr;
  /**
   * @brief Maximum timestep.
   */
  double maxTimestep{std::numeric_limits<double>::max()};
  /**
   * @brief Enable multithreading for pixel solver.
   */
  bool enableMultiThreading{false};
  /**
   * @brief Max thread count (0 means automatic/default).
   */
  std::size_t maxThreads{0};
  /**
   * @brief Enable common subexpression elimination in symbolic expressions.
   */
  bool doCSE{true};
  /**
   * @brief LLVM optimization level.
   */
  unsigned optLevel{3};

  template <class Archive>
  void serialize(Archive &ar, std::uint32_t const version) {
    if (version == 0) {
      ar(CEREAL_NVP(integrator), CEREAL_NVP(maxErr), CEREAL_NVP(maxTimestep),
         CEREAL_NVP(enableMultiThreading), CEREAL_NVP(maxThreads),
         CEREAL_NVP(doCSE), CEREAL_NVP(optLevel));
    } else if (version == 1) {
      ar(CEREAL_NVP(backend), CEREAL_NVP(gpuFloatPrecision),
         CEREAL_NVP(integrator), CEREAL_NVP(maxErr), CEREAL_NVP(maxTimestep),
         CEREAL_NVP(enableMultiThreading), CEREAL_NVP(maxThreads),
         CEREAL_NVP(doCSE), CEREAL_NVP(optLevel));
    }
  }
};

/**
 * @brief Global simulation options for all solver backends.
 */
struct Options {
  /**
   * @brief DUNE backend options.
   */
  DuneOptions dune;
  /**
   * @brief Pixel backend options.
   */
  PixelOptions pixel;

  template <class Archive>
  void serialize(Archive &ar, std::uint32_t const version) {
    if (version == 0) {
      ar(CEREAL_NVP(dune), CEREAL_NVP(pixel));
    }
  }
};

/**
 * @brief Aggregate concentration statistics.
 */
struct AvgMinMax {
  /**
   * @brief Average concentration.
   */
  double avg = 0;
  /**
   * @brief Minimum concentration.
   */
  double min = std::numeric_limits<double>::max();
  /**
   * @brief Maximum concentration.
   */
  double max = 0;

  template <class Archive>
  void serialize(Archive &ar, std::uint32_t const version) {
    if (version == 0) {
      ar(avg, min, max);
    }
  }
  /**
   * @brief Equality comparison.
   */
  friend bool operator==(const AvgMinMax &lhs, const AvgMinMax &rhs);
};

} // namespace sme::simulate

CEREAL_CLASS_VERSION(sme::simulate::Options, 0);
CEREAL_CLASS_VERSION(sme::simulate::DuneOptions, 2);
CEREAL_CLASS_VERSION(sme::simulate::PixelIntegratorError, 0);
CEREAL_CLASS_VERSION(sme::simulate::PixelOptions, 1);
CEREAL_CLASS_VERSION(sme::simulate::AvgMinMax, 0);

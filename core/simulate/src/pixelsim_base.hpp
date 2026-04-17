// Intermediate base for pixel simulator backends.
// Holds the adaptive-RK state and trivial error/stop-flag accessors that are
// identical across the CPU, CUDA, and Metal implementations.

#pragma once

#include "basesim.hpp"
#include "sme/image_stack.hpp"
#include "sme/simulate_options.hpp"
#include <atomic>
#include <cstddef>
#include <limits>
#include <string>

namespace sme::simulate {

class PixelSimBase : public BaseSim {
public:
  [[nodiscard]] const std::string &errorMessage() const override {
    return currentErrorMessage;
  }
  [[nodiscard]] const common::ImageStack &errorImages() const override {
    return currentErrorImages;
  }
  void setStopRequested(bool stop) override { stopRequested.store(stop); }
  [[nodiscard]] bool getStopRequested() const override {
    return stopRequested.load();
  }
  void setCurrentErrormessage(const std::string &msg) override {
    currentErrorMessage = msg;
  }

protected:
  PixelSimBase() = default;
  PixelSimBase(PixelIntegratorType integratorType,
               PixelIntegratorError errMaxTolerance, double maxTimestepValue)
      : integrator{integratorType}, errMax{errMaxTolerance},
        maxTimestep{maxTimestepValue} {}

  double maxStableTimestep{std::numeric_limits<double>::max()};
  std::size_t discardedSteps{0};
  PixelIntegratorType integrator{};
  PixelIntegratorError errMax{};
  double maxTimestep{std::numeric_limits<double>::max()};
  double nextTimestep{1e-7};
  double epsilon{1e-14};
  std::string currentErrorMessage{};
  common::ImageStack currentErrorImages{};
  std::atomic<bool> stopRequested{false};
};

} // namespace sme::simulate

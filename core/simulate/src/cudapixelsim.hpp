// CUDA pixel simulator PoC

#pragma once

#include "basesim.hpp"
#include "sme/simulate_options.hpp"
#include <atomic>
#include <cstddef>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace sme {

namespace model {
class Model;
}

namespace simulate {

class CudaPixelSim : public BaseSim {
private:
  struct Impl;
  std::unique_ptr<Impl> impl;
  const model::Model &doc;
  double maxStableTimestep{std::numeric_limits<double>::max()};
  PixelIntegratorType integrator;
  double maxTimestep{std::numeric_limits<double>::max()};
  std::string currentErrorMessage{};
  common::ImageStack currentErrorImages{};
  std::atomic<bool> stopRequested{false};

public:
  explicit CudaPixelSim(
      const model::Model &sbmlDoc,
      const std::vector<std::string> &compartmentIds,
      const std::vector<std::vector<std::string>> &compartmentSpeciesIds,
      const std::map<std::string, double, std::less<>> &substitutions = {});
  ~CudaPixelSim() override;

  std::size_t run(double time, double timeout_ms,
                  const std::function<bool()> &stopRunningCallback) override;

  [[nodiscard]] const std::vector<double> &
  getConcentrations(std::size_t compartmentIndex) const override;
  [[nodiscard]] std::size_t getConcentrationPadding() const override;
  [[nodiscard]] const std::vector<double> &
  getDcdt(std::size_t compartmentIndex) const;
  [[nodiscard]] double getLowerOrderConcentration(std::size_t compartmentIndex,
                                                  std::size_t speciesIndex,
                                                  std::size_t pixelIndex) const;
  [[nodiscard]] const std::string &errorMessage() const override;
  [[nodiscard]] const common::ImageStack &errorImages() const override;
  void setStopRequested(bool stop) override;
  [[nodiscard]] bool getStopRequested() const override;
  void setCurrentErrormessage(const std::string &msg) override;
};

} // namespace simulate

} // namespace sme

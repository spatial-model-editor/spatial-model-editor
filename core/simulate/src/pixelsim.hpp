// Pixel simulator

#pragma once

#include "basesim.hpp"
#include "sme/simulate_options.hpp"
#include <QImage>
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

class SimCompartment;
class SimMembrane;

class PixelSim : public BaseSim {
private:
  double maxStableTimestep{std::numeric_limits<double>::max()};
  void doRK101(double dt);
  void doRK212(double dt);
  void doRK323(double dt);
  void doRK435(double dt);
  void doRKSubstep(double dt, double g1, double g2, double g3, double beta,
                   double delta);
  double doRKAdaptive(double dtMax);
  PixelIntegratorType integrator;
  PixelIntegratorError errMax;
  double maxTimestep{std::numeric_limits<double>::max()};
  double nextTimestep{1e-7};
  common::ImageStack currentErrorImages{};
  std::size_t nExtraVars{0};

protected:
  bool useTBB{false};
  const model::Model &doc;
  std::vector<std::unique_ptr<SimCompartment>> simCompartments;
  std::vector<std::unique_ptr<SimMembrane>> simMembranes;
  void calculateDcdt();
  std::size_t discardedSteps{0};
  double epsilon{1e-14};
  std::string currentErrorMessage{};
  std::atomic<bool> stopRequested{false};
  std::size_t numMaxThreads{1};

public:
  explicit PixelSim(
      const model::Model &sbmlDoc,
      const std::vector<std::string> &compartmentIds,
      const std::vector<std::vector<std::string>> &compartmentSpeciesIds,
      const std::map<std::string, double, std::less<>> &substitutions = {});
  virtual ~PixelSim() override;
  double run_step(double time, double tNow) override;
  virtual std::size_t
  run(double time, double timeout_ms,
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
};

} // namespace simulate

} // namespace sme

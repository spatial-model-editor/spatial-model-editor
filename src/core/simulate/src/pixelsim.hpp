// Pixel simulator

#pragma once

#include "basesim.hpp"
#include "simulate_options.hpp"
#include <atomic>
#include <cstddef>
#include <limits>
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
  std::vector<std::unique_ptr<SimCompartment>> simCompartments;
  std::vector<std::unique_ptr<SimMembrane>> simMembranes;
  const model::Model &doc;
  double maxStableTimestep{std::numeric_limits<double>::max()};
  void calculateDcdt();
  void doRK101(double dt);
  void doRK212(double dt);
  void doRK323(double dt);
  void doRK435(double dt);
  void doRKSubstep(double dt, double g1, double g2, double g3, double beta,
                   double delta);
  double doRKAdaptive(double dtMax);
  double doTimestep(double dt, double dtMax);
  std::size_t discardedSteps{0};
  PixelIntegratorType integrator;
  PixelIntegratorError errMax;
  double maxTimestep{std::numeric_limits<double>::max()};
  double nextTimestep{1e-7};
  double epsilon{1e-14};
  bool useTBB{false};
  std::size_t numMaxThreads{1};
  std::string currentErrorMessage;
  std::atomic<bool> stopRequested{false};
  std::size_t nExtraVars{0};

public:
  explicit PixelSim(
      const model::Model &sbmlDoc,
      const std::vector<std::string> &compartmentIds,
      const std::vector<std::vector<std::string>> &compartmentSpeciesIds,
      const PixelOptions &pixelOptions = {});
  ~PixelSim() override;
  std::size_t run(double time) override;
  const std::vector<double> &
  getConcentrations(std::size_t compartmentIndex) const override;
  std::size_t getConcentrationPadding() const override;
  double getLowerOrderConcentration(std::size_t compartmentIndex,
                                    std::size_t speciesIndex,
                                    std::size_t pixelIndex) const;
  virtual const std::string &errorMessage() const override;
  virtual void requestStop() override;
};

} // namespace simulate

} // namespace sme

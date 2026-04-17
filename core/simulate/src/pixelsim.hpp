// Pixel simulator

#pragma once

#include "pixelsim_base.hpp"
#include <QImage>
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

/**
 * @brief Finite-difference pixel simulation backend.
 */
class PixelSim : public PixelSimBase {
private:
  std::vector<std::unique_ptr<SimCompartment>> simCompartments;
  std::vector<std::unique_ptr<SimMembrane>> simMembranes;
  const model::Model &doc;
  void calculateDcdt();
  void solveZeroStorageConstraints();
  double doRK101(double dt);
  void doRK212(double dt);
  void doRK323(double dt);
  void doRK435(double dt);
  void doRKSubstep(double dt, double g1, double g2, double g3, double beta,
                   double delta);
  double doRKAdaptive(double dtMax);
  bool hasAnyZeroStorageSpecies{false};
  double maxRelaxStableTimestep{std::numeric_limits<double>::max()};
  bool useTBB{false};
  std::size_t numMaxThreads{1};
  std::size_t nExtraVars{0};

public:
  /**
   * @brief Construct pixel simulator for selected compartments/species.
   */
  explicit PixelSim(
      const model::Model &sbmlDoc,
      const std::vector<std::string> &compartmentIds,
      const std::vector<std::vector<std::string>> &compartmentSpeciesIds,
      const std::map<std::string, double, std::less<>> &substitutions = {});
  /**
   * @brief Destructor.
   */
  ~PixelSim() override;
  /**
   * @brief Run simulation segment.
   */
  std::size_t run(double time, double timeout_ms,
                  const std::function<bool()> &stopRunningCallback) override;
  /**
   * @brief Concentration array for compartment.
   */
  [[nodiscard]] const std::vector<double> &
  getConcentrations(std::size_t compartmentIndex) const override;
  /**
   * @brief Concentration array padding.
   */
  [[nodiscard]] std::size_t getConcentrationPadding() const override;
  /**
   * @brief Time derivative array for compartment.
   */
  [[nodiscard]] const std::vector<double> &
  getDcdt(std::size_t compartmentIndex) const;
  /**
   * @brief Lower-order concentration value for adaptive RK.
   */
  [[nodiscard]] double getLowerOrderConcentration(std::size_t compartmentIndex,
                                                  std::size_t speciesIndex,
                                                  std::size_t pixelIndex) const;
};

} // namespace simulate

} // namespace sme

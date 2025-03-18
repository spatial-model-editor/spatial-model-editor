#pragma once
#include "pixelsim.hpp"
#include "steadystate_helper.hpp"

namespace sme {

namespace simulate {

/**
 * @brief TODO
 *
 */
class PixelSimSteadyState final : public PixelSim, public SteadyStateHelper {
  double stop_tolerance;

public:
  [[nodiscard]] double getStopTolerance() const override;

  /**
   * @brief Get the Concentrations object
   *
   * @return std::vector<double>
   */
  std::vector<double> getConcentrations() const override;

  /**
   * @brief Compute the stopping criterion as ||dc/dt|| / ||c||.
   *
   * @param c_old
   * @param c_new
   * @param dt
   * @return double
   */
  double compute_stopping_criterion(const std::vector<double> &c_old,
                                    const std::vector<double> &c_new,
                                    double dt) override;

  /**
   * @brief Construct a new PixelSimSteadyState object
   *
   * @param sbmlDoc
   * @param compartmentIds
   * @param compartmentSpeciesIds
   * @param substitutions
   * @param meta_dt
   * @param stop_tolerance
   */
  PixelSimSteadyState(
      const model::Model &sbmlDoc,
      const std::vector<std::string> &compartmentIds,
      const std::vector<std::vector<std::string>> &compartmentSpeciesIds,
      double stop_tolerance,
      const std::map<std::string, double, std::less<>> &substitutions = {});

  /**
   * @brief Run the simulation until a steady state is reached
   *
   * @param timeout_ms
   * @param stopRunningCallback
   * @return double the final value for the maximum dc_i/dt in any compartment
   */
  double run(double timeout_ms,
             const std::function<bool()> &stopRunningCallback) override;
};

} // namespace simulate
} // namespace sme

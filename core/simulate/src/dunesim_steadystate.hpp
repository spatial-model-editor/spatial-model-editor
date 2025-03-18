#pragma once
#include "dunesim.hpp"
#include "sme/duneconverter.hpp"
#include "steadystate_helper.hpp"
namespace sme {
namespace simulate {

/**
 * @brief TODO
 *
 */
class DuneSimSteadyState final : public DuneSim, public SteadyStateHelper {
  double stop_tolerance;

public:
  /**
   * @brief Construct a new Dune Sim Steady State object
   *
   * @param sbmlDoc ?
   * @param compartmentIds ?
   * @param stop_tolerance Tolerance level below which the derivative is
   * considered zero
   * @param meta_dt Pseudo timestep for the steady state simulation
   * @param substitutions ?
   */
  explicit DuneSimSteadyState(
      const model::Model &sbmlDoc,
      const std::vector<std::string> &compartmentIds, double stop_tolerance,
      const std::map<std::string, double, std::less<>> &substitutions = {});

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
   * @brief Get the Concentrations object
   *
   * @return std::vector<double>
   */
  std::vector<double> getConcentrations() const override;

  /**
   * @brief Run the simulation until steady state is reached.
   *
   * @param timeout_ms
   * @param stopRunningCallback
   * @return double The final value for the maximum of dc_i/dt over all
   * compartments
   */
  double run(double timeout_ms,
             const std::function<bool()> &stopRunningCallback) override;

  /**
   * @brief Get the number below which dc/dt is considered zero during a run
   *
   * @return double stop tolerance
   */
  double getStopTolerance() const override;
};

} // namespace simulate
} // namespace sme

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
  std::vector<double> old_state;
  std::vector<double> dcdt;
  double meta_dt;

  void updateOldState();

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
      double meta_dt = 1.0,
      const std::map<std::string, double, std::less<>> &substitutions = {});

  /**
   * @brief Calculate a first order estimate of the time derivative of the
   * concentrations.
   *
   */
  void compute_spatial_dcdt();

  /**
   * @brief Get the time derivative object
   *
   * @return std::vector<double>
   */
  std::vector<double> getDcdt() const override;

  /**
   * @brief Run the simulation until steady state is reached.
   *
   * @param timeout_ms
   * @param stopRunningCallback
   * @return std::size_t
   */
  std::size_t run(double timeout_ms,
                  const std::function<bool()> &stopRunningCallback) override;
};

} // namespace simulate
} // namespace sme

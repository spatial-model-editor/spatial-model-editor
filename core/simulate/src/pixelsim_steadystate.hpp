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
  std::vector<double> dcdt;
  std::vector<double> old_state;
  double meta_dt; // pseudo timestep for the steady state simulation
  double stop_tolerance;

public:
  /**
   * @brief Get the Dcdt object
   *
   * @return std::vector<double>
   */
  [[nodiscard]] std::vector<double> getDcdt() const override;

  [[nodiscard]] const std::vector<double> &getOldState() const;

  [[nodiscard]] double getMetaDt() const;

  [[nodiscard]] double getStopTolerance() const;

  /**
   * @brief Compute dc/dt over all compartments, but without spatial averaging.
   * This is needed for steady state calculations because averaging might drown
   * out some still evolving parts of the domain if all the rest is already at
   * steady state.
   *
   */
  void compute_spatial_dcdt();

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
      double meta_dt, double stop_tolerance,
      const std::map<std::string, double, std::less<>> &substitutions = {});

  /**
   * @brief TODO
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

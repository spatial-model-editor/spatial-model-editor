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
  double current_error;
  std::size_t steps_within_tolerance;
  std::size_t num_steps_steadystate;

public:
  /**
   * @brief Get the stop_tolerance
   *
   * @return double stop_tolerance below which the simulations is assumed to
   * have converged
   */
  [[nodiscard]] double getStopTolerance() const override;

  /**
   * @brief Set the stop_tolerance
   *
   * @param stop_tolerance New tolerance level below which the time derivative
   * is considered zero and the simulation is assumed to have converged
   */
  void setStopTolerance(double stop_tolerance) override;

  /**
   * @brief Get the number of timesteps for which the error was below the
   * tolerance to consider the current state a steady state
   *
   * @return std::size_t
   */
  [[nodiscard]] std::size_t getNumStepsSteady() const override;

  /**
   * @brief Set the Num Steps Steady object
   *
   */
  void setNumStepsSteady(std::size_t new_numstepssteady) override;

  /**
   * @brief Get the Concentrations object
   *
   * @return std::vector<double>
   */
  [[nodiscard]] std::vector<double> getConcentrations() const override;

  /**
   * @brief Get the number of timesteps for which the error was below the
   * tolerance set
   *
   * @return std::size_t
   */
  [[nodiscard]] std::size_t getStepsBelowTolerance() const override;

  /**
   * @brief Get the current error of the solver
   *
   */
  [[nodiscard]] double getCurrentError() const override;

  /**
   * @brief Check if the simulation has converged
   *
   * @return true has converged: steps_within_tolerance >= num_steps_steadystate
   * @return false has not yet converged
   */
  [[nodiscard]] bool hasConverged() const override;

  /**
   * @brief Compute the stopping criterion as ||dc/dt|| / ||c||.
   *
   * @param c_old
   * @param c_new
   * @param dt
   * @return double
   */
  [[nodiscard]] double
  computeStoppingCriterion(const std::vector<double> &c_old,
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
   * @return std::size_t the final value for the maximum dc_i/dt in any
   * compartment
   */
  [[nodiscard]] std::size_t
  run(double time, double timeout_ms,
      const std::function<bool()> &stopRunningCallback) override;
};

} // namespace simulate
} // namespace sme

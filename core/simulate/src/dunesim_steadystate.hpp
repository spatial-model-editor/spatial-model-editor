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
  double current_error;
  std::size_t steps_within_tolerance;
  std::size_t num_steps_steadystate;

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
   * @brief Geth the current error of the solver
   *
   */
  double getCurrentError() const override;

  /**
   * @brief Get the number of timesteps for which the error was below the
   * tolerance to consider the current state a steady state
   *
   * @return std::size_t
   */
  std::size_t getNumStepsSteady() const override;

  /**
   * @brief Set the Num Steps Steady object
   *
   */
  void setNumStepsSteady(std::size_t new_numstepssteady) override;

  /**
   * @brief Get the number of timesteps for which the error was below the
   * tolerance
   *
   * @return std::size_t
   */
  std::size_t getStepsBelowTolerance() const override;

  /**
   * @brief Set the stop_tolerance
   *
   * @param stop_tolerance New tolerance level below which the time derivative
   * is considered zero and the simulation is assumed to have converged
   */
  void setStopTolerance(double stop_tolerance) override;

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
  double computeStoppingCriterion(const std::vector<double> &c_old,
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
   * @param time
   * @param timeout_ms
   * @param stopRunningCallback
   * @return std::size_t Number of iterations taken until stop
   */
  std::size_t run(double time, double timeout_ms,
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

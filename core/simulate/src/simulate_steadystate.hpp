#include "basesim.hpp"
#include "sme/model.hpp"
#include "sme/simulate_options.hpp"

#include <cstddef>
#include <qrgb.h>

namespace sme {
namespace simulate {
enum class SteadystateConvergenceMode { absolute, relative };
class SteadyStateSimulation final {

  // data members
  bool m_has_converged;
  sme::model::Model &m_model;
  std::unique_ptr<BaseSim> m_simulator;
  double m_convergenceTolerance;
  std::size_t m_steps_below_tolerance;
  std::size_t m_steps_to_convergence;
  double m_timeout_ms;
  SteadystateConvergenceMode m_stop_mode;
  std::vector<double> m_steps;
  std::vector<double> m_errors;
  std::vector<int> m_compartmentIdxs;
  std::vector<std::string> m_compartmentIds;
  std::vector<std::vector<std::string>> m_compartmentSpeciesIds;
  std::vector<std::vector<QRgb>> m_compartmentSpeciesColors;
  double m_dt; // timestep to check for convergence, not solver timestep

  // helper functions for solvers
  void initModel();
  void reset_solver();

  // .. and for running them
  void runDune(double time);
  void runPixel(double time);
  [[nodiscard]] double
  computeStoppingCriterion(const std::vector<double> &c_old,
                           const std::vector<double> &c_new, double dt);

  // helper functions for data
  void append_data(double timestep, double error);
  void reset_data();

public:
  // lifecycle

  /**
   * @brief Construct a new Steady State Simulation object
   *
   * @param model Model to use
   * @param type SimulatorType to use: Dune or Pixel
   * @param tolerance tolerance for convergence the stopping criterion must be
   * smaller than this to be considered converged
   * @param steps_to_convergence Number of timesteps the simulation needs to
   * have a stable solution for to consider it converged
   * @param convergence_mode How to compute the convergence criterion: absolute
   * or relative
   * @param timeout_ms Number of miliseconds the simulation is allowed to run
   * before stopping
   * @param dt Timestep to check for convergence (not solver timestep, this is
   * set independently by the solver itself (!!))
   */
  SteadyStateSimulation(sme::model::Model &model, SimulatorType type,
                        double tolerance, std::size_t steps_to_convergence,
                        SteadystateConvergenceMode convergence_mode,
                        std::size_t timeout_ms, double dt = 1e-2);
  ~SteadyStateSimulation() = default;

  // getters

  /**
   * @brief Check if the simulation has converged
   *
   * @return true if has converged according to stoppingcriterion and tolerance
   * @return false otherwise
   */
  [[nodiscard]] bool hasConverged() const;

  /**
   * @brief Get the convergence mode
   *
   * @return SteadystateConvergenceMode
   */
  [[nodiscard]] SteadystateConvergenceMode getConvergenceMode();

  /**
   * @brief Get the number of steps below tolerance required to consider the
   * simulation converged
   *
   * @return std::size_t
   */
  [[nodiscard]] std::size_t getStepsBelowTolerance() const;

  /**
   * @brief Get the type of simulator used
   *
   * @return SimulatorType: Dune or Pixel
   */
  [[nodiscard]] SimulatorType getSimulatorType();

  /**
   * @brief Get the tolerance used to determine convergence
   *
   * @return double
   */
  [[nodiscard]] double getStopTolerance() const;

  /**
   * @brief Get the concentrations of all species in all compartments
   *
   * @return std::vector<double>
   */
  [[nodiscard]] std::vector<double> getConcentrations() const;

  /**
   * @brief Get the current error
   *
   * @return double The current value of the quantity computed in the stopping
   * criterion
   */
  [[nodiscard]] double getCurrentError() const;

  /**
   * @brief Get the current time of the simulation
   *
   * @return double current times
   */
  [[nodiscard]] double getCurrentStep() const;

  /**
   * @brief Get the number of steps the simulation needs to have a stable
   * solution for to consider it converged
   *
   * @return std::size_t
   */
  [[nodiscard]] std::size_t getStepsToConvergence() const;

  /**
   * @brief Get the steps taken in the simulation to check for convergence
   *
   * @return const std::vector<double>&
   */
  [[nodiscard]] const std::vector<double> &getSteps() const;

  /**
   * @brief Get the errors at each step in a dvector
   *
   * @return const std::vector<double>&
   */
  [[nodiscard]] const std::vector<double> &getErrors() const;

  /**
   * @brief Get the timestep used to check for convergence
   *
   * @return double
   */
  [[nodiscard]] double getDt() const;

  // setters
  void setStopMode(SteadystateConvergenceMode mode);
  void setStepsBelowTolerance(std::size_t new_numstepssteady);
  void setStopTolerance(double stop_tolerance);
  void setSimulatorType(SimulatorType type);
  void setStepsToConvergence(std::size_t steps_to_convergence);
  void setDt(double dt);
  // functionality
  void run(double time);
  void selectSimulator();
};
} // namespace simulate
} // namespace sme

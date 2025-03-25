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
  double m_convergence_tolerance;
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
  void selectSimulator();
  // .. and for running them
  void runDune(double time);
  void runPixel(double time);
  [[nodiscard]] double
  computeStoppingCriterion(const std::vector<double> &c_old,
                           const std::vector<double> &c_new);

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

  /**
   * @brief Get the error message from the solvers
   *
   * @return std::string
   */
  [[nodiscard]] std::string getSolverErrormessage() const;

  /**
   * @brief Get a boolean flag if the solver has been requested to stop
   *
   * @return true Solver has been requested to be stopped
   * @return false otherwise
   */
  [[nodiscard]] bool getSolverStopRequested() const;

  /**
   * @brief Get the timeout threshold in milliseconds
   *
   * @return double
   */
  [[nodiscard]] double getTimeout() const;

  // setters

  /**
   * @brief Set the convergence mode
   *
   * @param mode SteadystateConvergenceMode: absolute or relative
   */
  void setStopMode(SteadystateConvergenceMode mode);

  /**
   * @brief Set the number of steps below tolerance required to consider the
   * simulation converged
   *
   * @param new_numstepssteady std::size_t
   */
  void setStepsBelowTolerance(std::size_t new_numstepssteady);

  /**
   * @brief Set the tolerance used to determine convergence
   *
   * @param stop_tolerance double
   */
  void setStopTolerance(double stop_tolerance);

  /**
   * @brief Set the type of simulator used. This resets all data and solver
   * states and creates a new simulation with the new solver type
   *
   * @param type SimulatorType: Dune or Pixel
   */
  void setSimulatorType(SimulatorType type);

  /**
   * @brief Set the number of steps the simulation needs to have a stable
   * solution for to consider it converged
   *
   * @param steps_to_convergence std::size_t
   */
  void setStepsToConvergence(std::size_t steps_to_convergence);

  /**
   * @brief Set the timestep to check for convergence
   *
   * @param dt
   */
  void setDt(double dt);

  /**
   * @brief Set the timeout threshold in milliseconds
   *
   * @param timeout_ms
   */
  void setTimeout(double timeout_ms);

  // functionality

  /**
   * @brief Run the simulation for the given time in seconds
   *
   * @param time_s
   */
  void run();
};
} // namespace simulate
} // namespace sme

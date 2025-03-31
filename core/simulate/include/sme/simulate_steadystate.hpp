#include "../../src/basesim.hpp"
#include "sme/image_stack.hpp"
#include "sme/model.hpp"
#include "sme/simulate_options.hpp"
#include <cstddef>

namespace sme {
namespace simulate {
enum class SteadystateConvergenceMode { absolute, relative };

struct SteadyStateData {
  double step;
  double error;
};
class SteadyStateSimulation final {

  // data members for simulation
  std::atomic<bool> m_has_converged;
  sme::model::Model &m_model;
  std::unique_ptr<BaseSim> m_simulator;
  double m_convergence_tolerance;
  std::size_t m_steps_below_tolerance;
  std::size_t m_steps_to_convergence;
  double m_timeout_ms;
  SteadystateConvergenceMode m_stop_mode;
  double m_dt; // timestep to check for convergence, not solver timestep
  std::mutex m_concentration_mutex;

  // data members for plotting
  std::atomic<std::shared_ptr<SteadyStateData>> m_data;
  std::vector<const geometry::Compartment *> m_compartments;
  std::vector<std::string> m_compartmentIds;
  std::vector<std::size_t> m_compartmentIndices;
  std::vector<std::vector<std::string>> m_compartmentSpeciesIds;
  std::vector<std::vector<std::size_t>> m_compartmentSpeciesIdxs;
  std::vector<std::vector<QRgb>> m_compartmentSpeciesColors;

  // helper functions for solvers
  void initModel();
  void resetModel();
  void resetSolver();
  void selectSimulator();

  // .. and for running them
  void runDune(double time);
  void runPixel(double time);
  [[nodiscard]] double
  computeStoppingCriterion(const std::vector<double> &c_old,
                           const std::vector<double> &c_new);

  // helper functions for data
  std::vector<std::vector<double>> computeConcentrationNormalisation(
      const std::vector<std::vector<std::size_t>> &speciesToDraw,
      bool normaliseOverAllSpecies) const;
  void recordData(double timestep, double error);
  void resetData();

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
   * @brief Get the latest data point
   *
   * @return const std::atomic<std::shared_ptr<SteadyStateData>> &
   A pointer to a struct containing the step, error and
   * concentration image stack of the latest step of the simulation.
   */
  [[nodiscard]] const std::atomic<std::shared_ptr<SteadyStateData>> &
  getLatestData() const;

  /**
   * @brief Get the number of steps the simulation needs to have a stable
   * solution for to consider it converged
   *
   * @return std::size_t
   */
  [[nodiscard]] std::size_t getStepsToConvergence() const;

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

  /**
   * @brief Get the Compartment Species Ids object
   *
   * @return std::vector<std::vector<std::size_t>>
   */
  [[nodiscard]] std::vector<std::vector<std::size_t>>
  getCompartmentSpeciesIdxs() const;

  /**
   * @brief Get the Compartment Species Colors object
   *
   * @return std::vector<std::vector<QRgb>>
   */
  [[nodiscard]] std::vector<std::vector<QRgb>>
  getCompartmentSpeciesColors() const;

  /**
   * @brief Get the Compartment Species Ids object
   *
   * @return std::vector<std::vector<std::string>>
   */
  [[nodiscard]] std::vector<std::vector<std::string>>
  getCompartmentSpeciesIds() const;

  /**
   * @brief Get the concentration image stack for a given timepoint
   *
   * @param speciesToDraw vector of lists of species indices that should be
   * included into the image
   * @param normaliseOverAllSpecies bool: if true, the image will be normalised
   * over all species
   * @return common::ImageStack
   */
  [[nodiscard]] common::ImageStack getConcentrationImage(
      const std::vector<std::vector<std::size_t>> &speciesToDraw,
      bool normaliseOverAllSpecies);

  // setters

  /**
   * @brief Set the convergence mode
   *
   * @param mode SteadystateConvergenceMode: absolute or relative
   */
  void setConvergenceMode(SteadystateConvergenceMode mode);

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

  /**
   * @brief Request the simulation to stop
   *
   */
  void requestStop();

  /**
   * @brief Reset the solver to its initial state. This gets rid of all data and
   * solver states
   *
   */
  void reset();
};
} // namespace simulate
} // namespace sme

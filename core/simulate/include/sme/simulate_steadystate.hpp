#include "sme/image_stack.hpp"
#include "sme/model.hpp"
#include "sme/simulate_options.hpp"
#include <cstddef>

namespace sme::simulate {
class BaseSim;
enum class SteadyStateConvergenceMode { absolute, relative };

struct SteadyStateData {
  double step;
  double error;
};
class SteadyStateSimulation final {

  // data members for simulation
  std::atomic<bool> m_has_converged = false;
  std::atomic<bool> m_stop_requested = false;
  sme::model::Model &m_model;
  std::unique_ptr<BaseSim> m_simulator;
  double m_convergence_tolerance;
  std::size_t m_steps_below_tolerance = 0;
  std::size_t m_steps_to_convergence;
  double m_timeout_ms;
  SteadyStateConvergenceMode m_stop_mode;
  double m_dt; // timestep to check for convergence, not solver timestep
  std::mutex m_concentration_mutex = std::mutex();

  // data members for plotting
  std::atomic<double> m_error = std::numeric_limits<double>::max();
  std::atomic<double> m_step = 0.0;
  std::vector<double> m_concentrations = {};
  std::vector<const geometry::Compartment *> m_compartments = {};
  std::vector<std::string> m_compartmentIds = {};
  std::vector<std::size_t> m_compartmentIndices = {};
  std::vector<std::vector<std::string>> m_compartmentSpeciesIds = {};
  std::vector<std::vector<std::size_t>> m_compartmentSpeciesIdxs = {};
  std::vector<std::vector<QRgb>> m_compartmentSpeciesColors = {};

  // helper functions for solvers
  void initModel();
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
   * set independently by the solver itself (!))
   */
  SteadyStateSimulation(sme::model::Model &model, SimulatorType type,
                        double tolerance, std::size_t steps_to_convergence,
                        SteadyStateConvergenceMode convergence_mode,
                        double timeout_ms, double dt);
  ~SteadyStateSimulation();

  // getters

  /**
   * @brief Check if the simulation has converged
   *
   * @return const std::atomic<bool>&
   */
  [[nodiscard]] const std::atomic<bool> &hasConverged() const;

  /**
   * @brief Check if the simulation has been requested to stop
   *
   * @return const std::atomic<bool>&
   */
  [[nodiscard]] const std::atomic<bool> &getStopRequested() const;

  /**
   * @brief Get the convergence mode
   *
   * @return SteadystateConvergenceMode
   */
  [[nodiscard]] SteadyStateConvergenceMode getConvergenceMode() const;

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
  [[nodiscard]] SimulatorType getSimulatorType() const;

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
  [[nodiscard]] const std::vector<double> &getConcentrations();

  /**
   * @brief Get the latest error value of the simulation
   *
   * @return const std::atomic<double> &

   A pointer to a struct containing the step, error and
   * concentration image stack of the latest step of the simulation.
   */
  [[nodiscard]] const std::atomic<double> &getLatestError() const;

  /**
   * @brief Get the latest simulation step
   *
   * @return const std::atomic<double>&
   */
  [[nodiscard]] const std::atomic<double> &getLatestStep() const;

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
  void setConvergenceMode(SteadyStateConvergenceMode mode);

  /**
   * @brief Set the tolerance used to determine convergence
   *
   * @param stop_tolerance double
   */
  void setStopTolerance(double stop_tolerance);

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
} // namespace sme::simulate

#include "basesim.hpp"
#include "sme/model.hpp"
#include "sme/simulate_options.hpp"

#include <cstddef>

namespace sme {
namespace simulate {
enum class SteadystateStopMode { absolute, relative };
class SteadyStateSimulation final {
  std::atomic<bool> m_has_converged;
  sme::model::Model &m_model;
  std::unique_ptr<BaseSim> m_simulator;
  double m_convergenceTolerance;
  std::size_t m_steps_below_tolerance;
  std::size_t m_steps_to_convergence;
  double m_timeout_ms;
  SteadystateStopMode m_stop_mode;
  std::vector<double> m_steps;
  std::vector<double> m_errors;
  void runDune(double time);
  void runPixel(double time);

public:
  SteadyStateSimulation(sme::model::Model &model, SimulatorType type,
                        double tolerance, std::size_t timeout_ms);
  ~SteadyStateSimulation() = default;

  [[nodiscard]] bool hasConverged() const;
  [[nodiscard]] SteadystateStopMode getStopMode();
  [[nodiscard]] std::size_t getStepsBelowTolerance() const;
  [[nodiscard]] SimulatorType getSimulatorType();
  [[nodiscard]] double getStopTolerance() const;
  [[nodiscard]] std::vector<double> getConcentrations() const;
  [[nodiscard]] double getCurrentError() const;
  [[nodiscard]] double getCurrentStep() const;
  [[nodiscard]] std::size_t getStepsToConvergence() const;
  [[nodiscard]] const std::vector<double> &getSteps() const;
  [[nodiscard]] const std::vector<double> &getErrors() const;

  void setStopMode(SteadystateStopMode mode);
  void setStepsBelowTolerance(std::size_t new_numstepssteady);
  void setStopTolerance(double stop_tolerance);
  void setSimulatorType(SimulatorType type);
  void setStepsToConvergence(std::size_t steps_to_convergence);

  void run(double time);
  void selectSimulator();
  [[nodiscard]] double
  computeStoppingCriterion(const std::vector<double> &c_old,
                           const std::vector<double> &c_new, double dt);
};
} // namespace simulate
} // namespace sme

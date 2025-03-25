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

  // helper functions
  void initModel();
  void runDune(double time);
  void runPixel(double time);

public:
  // lifecycle
  SteadyStateSimulation(sme::model::Model &model, SimulatorType type,
                        double tolerance, std::size_t steps_to_convergence,
                        SteadystateConvergenceMode convergence_mode,
                        std::size_t timeout_ms, double dt = 1e-2);
  ~SteadyStateSimulation() = default;

  // state getters and setters
  [[nodiscard]] bool hasConverged() const;
  [[nodiscard]] SteadystateConvergenceMode getStopMode();
  [[nodiscard]] std::size_t getStepsBelowTolerance() const;
  [[nodiscard]] SimulatorType getSimulatorType();
  [[nodiscard]] double getStopTolerance() const;
  [[nodiscard]] std::vector<double> getConcentrations() const;
  [[nodiscard]] double getCurrentError() const;
  [[nodiscard]] double getCurrentStep() const;
  [[nodiscard]] std::size_t getStepsToConvergence() const;
  [[nodiscard]] const std::vector<double> &getSteps() const;
  [[nodiscard]] const std::vector<double> &getErrors() const;
  [[nodiscard]] double getDt() const;

  void setStopMode(SteadystateConvergenceMode mode);
  void setStepsBelowTolerance(std::size_t new_numstepssteady);
  void setStopTolerance(double stop_tolerance);
  void setSimulatorType(SimulatorType type);
  void setStepsToConvergence(std::size_t steps_to_convergence);
  void setDt(double dt);
  // functionality
  void run(double time);
  void selectSimulator();
  [[nodiscard]] double
  computeStoppingCriterion(const std::vector<double> &c_old,
                           const std::vector<double> &c_new, double dt);
};
} // namespace simulate
} // namespace sme

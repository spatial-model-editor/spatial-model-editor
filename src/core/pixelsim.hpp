// Pixel simulator
//  - ReacEval: evaluates reaction terms at a single location
//  - SimCompartment: evaluates reactions in a compartment
//  - SimMembrane: evaluates reactions in a membrane
//  - PixelSim: forwards Euler simulator

#pragma once

#include <QImage>

#include "basesim.hpp"
#include "symbolic.hpp"

namespace sbml {
class SbmlDocWrapper;
}

namespace geometry {
class Compartment;
class Membrane;
}  // namespace geometry

namespace sim {

class ReacEval {
 private:
  // symengine reaction expression
  symbolic::Symbolic sym;
  // vector of result of evaluating reactions
  std::vector<double> result;

 public:
  std::size_t nSpecies = 0;
  ReacEval() = default;
  ReacEval(const sbml::SbmlDocWrapper &doc,
           const std::vector<std::string> &speciesID,
           const std::vector<std::string> &reactionID,
           const std::vector<std::string> &reactionScaleFactors);
  void evaluate(double *output, const double *input) const;
};

class SimCompartment {
 private:
  ReacEval reacEval;
  // species concentrations & corresponding dcdt values
  // ordering: ix, species
  std::vector<double> conc;
  std::vector<double> dcdt;
  std::vector<double> s2;
  std::vector<double> s3;
  // dimensionless diffusion constants for each species
  std::vector<double> diffConstants;
  const geometry::Compartment &comp;
  std::string compartmentId;
  std::vector<std::string> speciesIds;
  std::vector<std::size_t> nonSpatialSpeciesIndices;
  double maxStableTimestep = std::numeric_limits<double>::max();

 public:
  explicit SimCompartment(const sbml::SbmlDocWrapper &doc,
                          const geometry::Compartment &compartment,
                          const std::vector<std::string> &sIds);
  // dcdt = result of applying diffusion operator to conc
  void evaluateDiffusionOperator();
  // dcdt += result of applying reaction expressions to conc
  void evaluateReactions();
  void spatiallyAverageDcdt();
  void doForwardsEulerTimestep(double dt);
  void doRKInit();
  void doRKSubstep(double dt, double g1, double g2, double g3, double beta,
                   double delta);
  void doRKFinalise(double cFactor, double s2Factor, double s3Factor);
  void undoRKStep();
  IntegratorError calculateRKError(double epsilon) const;
  const std::string &getCompartmentId() const;
  const std::vector<std::string> &getSpeciesIds() const;
  const std::vector<double> &getConcentrations() const;
  double getLowerOrderConcentration(std::size_t speciesIndex,
                                    std::size_t pixelIndex) const;
  const std::vector<QPoint> &getPixels() const;
  std::vector<double> &getDcdt();
  double getMaxStableTimestep() const;
};

class SimMembrane {
 private:
  ReacEval reacEval;
  const geometry::Membrane &membrane;
  SimCompartment *compA;
  SimCompartment *compB;

 public:
  SimMembrane(const sbml::SbmlDocWrapper &doc,
              const geometry::Membrane &membrane_ptr, SimCompartment *simCompA,
              SimCompartment *simCompB);
  void evaluateReactions();
};

class PixelSim : public BaseSim {
 private:
  std::vector<SimCompartment> simCompartments;
  std::vector<SimMembrane> simMembranes;
  const sbml::SbmlDocWrapper &doc;
  double maxStableTimestep = std::numeric_limits<double>::max();
  void calculateDcdt();
  void doRK101(double dt);
  void doRK212(double dt);
  void doRK323(double dt);
  void doRK435(double dt);
  double doRKAdaptive(double dtMax);
  double doTimestep(double dt, double dtMax);
  std::vector<std::size_t> integratorOrders{1, 2, 3, 4};
  std::size_t integratorOrder = 2;
  std::size_t discardedSteps = 0;
  IntegratorError errMax{std::numeric_limits<double>::max(), 1e-2};
  double maxTimestep = std::numeric_limits<double>::max();
  double nextTimestep = 1e-7;
  double epsilon = 1e-14;

 public:
  explicit PixelSim(
      const sbml::SbmlDocWrapper &sbmlDoc,
      const std::vector<std::string> &compartmentIds,
      const std::vector<std::vector<std::string>> &compartmentSpeciesIds,
      std::size_t order = 2);
  ~PixelSim() override;
  virtual void setIntegrationOrder(std::size_t order) override;
  virtual std::size_t getIntegrationOrder() const override;
  virtual void setIntegratorError(const IntegratorError &err) override;
  virtual IntegratorError getIntegratorError() const override;
  virtual void setMaxDt(double maxDt) override;
  virtual double getMaxDt() const override;
  std::size_t run(double time) override;
  const std::vector<double> &getConcentrations(
      std::size_t compartmentIndex) const override;
  double getLowerOrderConcentration(std::size_t compartmentIndex,
                                    std::size_t speciesIndex,
                                    std::size_t pixelIndex) const override;
};

}  // namespace sim

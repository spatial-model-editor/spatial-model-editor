// Pixel simulator implementation
//  - ReacEval: evaluates reaction terms at a single location
//  - SimCompartment: evaluates reactions in a compartment
//  - SimMembrane: evaluates reactions in a membrane

#pragma once

#include "simulate_options.hpp"
#include "symbolic.hpp"
#include <cstddef>
#include <limits>
#include <string>
#include <vector>

class QPoint;

namespace model {
class Model;
}

namespace geometry {
class Compartment;
class Membrane;
} // namespace geometry

namespace simulate {

class ReacEval {
private:
  // symengine reaction expression
  symbolic::Symbolic sym;

public:
  ReacEval() = default;
  ReacEval(const model::Model &doc, const std::vector<std::string> &speciesID,
           const std::vector<std::string> &reactionID,
           const std::vector<std::string> &reactionScaleFactors);
  ReacEval(ReacEval &&) noexcept = default;
  ReacEval(const ReacEval &) = delete;
  ReacEval &operator=(ReacEval &&) noexcept = default;
  ReacEval &operator=(const ReacEval &) = delete;
  ~ReacEval() = default;
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
  const geometry::Compartment *comp;
  std::string compartmentId;
  std::vector<std::string> speciesIds;
  std::vector<std::size_t> nonSpatialSpeciesIndices;
  double maxStableTimestep = std::numeric_limits<double>::max();

public:
  explicit SimCompartment(const model::Model &doc,
                          const geometry::Compartment *compartment,
                          std::vector<std::string> sIds);
  SimCompartment(SimCompartment &&) noexcept = default;
  SimCompartment(const SimCompartment &) = delete;
  SimCompartment &operator=(SimCompartment &&) noexcept = default;
  SimCompartment &operator=(const SimCompartment &) = delete;
  ~SimCompartment() = default;

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
  PixelIntegratorError calculateRKError(double epsilon) const;
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
  const geometry::Membrane *membrane;
  SimCompartment *compA;
  SimCompartment *compB;

public:
  SimMembrane(const model::Model &doc, const geometry::Membrane *membrane_ptr,
              SimCompartment *simCompA, SimCompartment *simCompB);
  SimMembrane(SimMembrane &&) noexcept = default;
  SimMembrane(const SimMembrane &) = delete;
  SimMembrane &operator=(SimMembrane &&) noexcept = default;
  SimMembrane &operator=(const SimMembrane &) = delete;
  ~SimMembrane() = default;
  void evaluateReactions();
};

} // namespace simulate

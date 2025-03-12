// Pixel simulator implementation
//  - SimCompartment: evaluates reactions in a compartment
//  - SimMembrane: evaluates reactions in a membrane

#pragma once

#include "sme/image_stack.hpp"
#include "sme/pde.hpp"
#include "sme/simulate_options.hpp"
#include "sme/symbolic.hpp"
#include <QImage>
#include <QPoint>
#include <cstddef>
#include <limits>
#include <string>
#include <vector>

namespace sme {

namespace model {
class Model;
}

namespace geometry {
class Compartment;
class Membrane;
} // namespace geometry

namespace simulate {

struct ReacExpr {
  std::vector<std::string> expressions;
  std::vector<std::string> variables;
  ReacExpr(
      const model::Model &doc, const std::vector<std::string> &speciesID,
      const std::vector<std::string> &reactionID,
      double reactionScaleFactor = 1.0, bool timeDependent = false,
      bool spaceDependent = false,
      const std::map<std::string, double, std::less<>> &substitutions = {});
};

class SimCompartment {
private:
  common::Symbolic sym;
  // species concentrations & corresponding dcdt values
  // ordering: ix, species
  std::vector<double> conc;
  std::vector<double> dcdt;
  std::vector<double> s2;
  std::vector<double> s3;
  // dimensionless diffusion constants in x,y,z directions for each species
  // i.e. [{D/dx^2, D/dy^2, D/dz^2}, {}, .. ]
  std::vector<std::array<double, 3>> diffConstants;
  const geometry::Compartment *comp;
  std::size_t nPixels;
  std::size_t nSpecies;
  std::string compartmentId;
  std::vector<std::string> speciesIds;
  std::vector<std::string> speciesNames;
  std::vector<std::size_t> nonSpatialSpeciesIndices;
  double maxStableTimestep = std::numeric_limits<double>::max();

public:
  explicit SimCompartment(
      const model::Model &doc, const geometry::Compartment *compartment,
      std::vector<std::string> sIds, bool doCSE = true, unsigned optLevel = 3,
      bool timeDependent = false, bool spaceDependent = false,
      const std::map<std::string, double, std::less<>> &substitutions = {});

  // dcdt = result of applying diffusion operator to conc
  void evaluateDiffusionOperator(std::size_t begin, std::size_t end);
  // dcdt += result of applying reaction expressions to conc
  void evaluateReactions(std::size_t begin, std::size_t end);
  void evaluateReactionsAndDiffusion();
  void evaluateReactionsAndDiffusion_tbb();
  void spatiallyAverageDcdt();
  void doForwardsEulerTimestep(double dt, std::size_t begin, std::size_t end);
  void doForwardsEulerTimestep(double dt);
  void doForwardsEulerTimestep_tbb(double dt);
  void doRKInit();
  void doRK212Substep1(double dt, std::size_t begin, std::size_t end);
  void doRK212Substep1(double dt);
  void doRK212Substep1_tbb(double dt);
  void doRK212Substep2(double dt, std::size_t begin, std::size_t end);
  void doRK212Substep2(double dt);
  void doRK212Substep2_tbb(double dt);
  void doRKSubstep(double dt, double g1, double g2, double g3, double beta,
                   double delta, std::size_t begin, std::size_t end);
  void doRKSubstep(double dt, double g1, double g2, double g3, double beta,
                   double delta);
  void doRKSubstep_tbb(double dt, double g1, double g2, double g3, double beta,
                       double delta);
  void doRKFinalise(double cFactor, double s2Factor, double s3Factor,
                    std::size_t begin, std::size_t end);
  void doRKFinalise(double cFactor, double s2Factor, double s3Factor);
  void doRKFinalise_tbb(double cFactor, double s2Factor, double s3Factor);
  void undoRKStep(std::size_t begin, std::size_t end);
  void undoRKStep();
  void undoRKStep_tbb();
  [[nodiscard]] PixelIntegratorError calculateRKError(double epsilon) const;
  std::string plotRKError(common::ImageStack &images, double epsilon,
                          double max) const;
  [[nodiscard]] const std::string &getCompartmentId() const;
  [[nodiscard]] const std::vector<std::string> &getSpeciesIds() const;
  [[nodiscard]] const std::vector<double> &getConcentrations() const;
  void setConcentrations(const std::vector<double> &);
  [[nodiscard]] double getLowerOrderConcentration(std::size_t speciesIndex,
                                                  std::size_t pixelIndex) const;
  [[nodiscard]] const std::vector<common::Voxel> &getVoxels() const;
  std::vector<double> &getDcdt();
  [[nodiscard]] double getMaxStableTimestep() const;
  [[nodiscard]] double getNSpecies() const;
};

class SimMembrane {
private:
  common::Symbolic sym;
  const geometry::Membrane *membrane;
  SimCompartment *compA;
  SimCompartment *compB;
  common::VolumeF voxelSize{};
  std::size_t nExtraVars{0};

public:
  SimMembrane(
      const model::Model &doc, const geometry::Membrane *membrane_ptr,
      SimCompartment *simCompA, SimCompartment *simCompB, bool doCSE = true,
      unsigned optLevel = 3, bool timeDependent = false,
      bool spaceDependent = false,
      const std::map<std::string, double, std::less<>> &substitutions = {});
  void evaluateReactions();
};

} // namespace simulate

} // namespace sme

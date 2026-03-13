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
#include <array>
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

/**
 * @brief Reaction-expression bundle with variable ordering.
 */
struct ReacExpr {
  /**
   * @brief Expressions for each reaction/species equation.
   */
  std::vector<std::string> expressions;
  /**
   * @brief Variable names expected by expressions.
   */
  std::vector<std::string> variables;
  /**
   * @brief Build expression bundle from model and reaction ids.
   */
  ReacExpr(
      const model::Model &doc, const std::vector<std::string> &speciesID,
      const std::vector<std::string> &reactionID,
      double reactionScaleFactor = 1.0, bool timeDependent = false,
      bool spaceDependent = false,
      const std::map<std::string, double, std::less<>> &substitutions = {});
};

/**
 * @brief Pixel-domain simulation state for one compartment.
 */
class SimCompartment {
private:
  struct CrossDiffusionTerm {
    std::size_t targetSpeciesIndex{};
    std::size_t sourceSpeciesIndex{};
  };
  common::Symbolic sym;
  common::Symbolic symCrossDiffusion;
  // species concentrations & corresponding dcdt values
  // ordering: ix, species
  std::vector<double> conc;
  std::vector<double> dcdt;
  std::vector<double> s2;
  std::vector<double> s3;
  std::vector<CrossDiffusionTerm> crossDiffusionTerms;
  // cross-diffusion coefficients D_ij(x, c, t) for each pixel and configured
  // term (ordering: pixel, term)
  std::vector<double> crossDiffusionCoefficients;
  // diffusion constants (D) per voxel for each species
  std::vector<std::vector<double>> diffConstants;
  // diffusion constants (D/dx^2, D/dy^2, D/dz^2) per species
  std::vector<std::array<double, 3>> diffConstantsUniform;
  // inverse storage term (1 / S) per species
  std::vector<double> invStorage;
  const geometry::Compartment *comp;
  std::size_t nPixels;
  std::size_t nSpecies;
  std::string compartmentId;
  std::vector<std::string> speciesIds;
  std::vector<std::string> speciesNames;
  std::vector<std::size_t> nonSpatialSpeciesIndices;
  std::vector<std::size_t> zeroStorageSpeciesIndices;
  std::size_t nPrimarySpecies{0};
  std::vector<double> maxPrimaryDiagonalDiffusion;
  std::vector<double> relaxOld;
  std::vector<double> relaxFirstOrder;
  double maxStableTimestep = std::numeric_limits<double>::max();
  double maxRelaxStableTimestep = std::numeric_limits<double>::max();
  double dx2{1.0};
  double dy2{1.0};
  double dz2{1.0};
  bool useUniformDiffusionOperator{false};
  bool hasNonUnitStorage{false};
  bool hasZeroStorageSpecies{false};
  bool hasCrossDiffusion{false};

public:
  /**
   * @brief Construct compartment simulation state.
   */
  explicit SimCompartment(
      const model::Model &doc, const geometry::Compartment *compartment,
      std::vector<std::string> sIds, bool doCSE = true, unsigned optLevel = 3,
      bool timeDependent = false, bool spaceDependent = false,
      bool useUniformDiffusionOperator = false,
      const std::map<std::string, double, std::less<>> &substitutions = {});

  /**
   * @brief Evaluate diffusion contribution into ``dcdt`` for voxel range.
   */
  void evaluateDiffusionOperator(std::size_t begin, std::size_t end);
  /**
   * @brief Add reaction contribution into ``dcdt`` for voxel range.
   */
  void evaluateReactions(std::size_t begin, std::size_t end);
  /**
   * @brief Evaluate cross-diffusion coefficients for voxel range.
   */
  void evaluateCrossDiffusionCoefficients(std::size_t begin, std::size_t end);
  /**
   * @brief Update stable timestep bound from cross diffusion terms.
   */
  void updateCrossDiffusionMaxStableTimestep();
  /**
   * @brief Apply cross-diffusion operator for voxel range.
   */
  void evaluateCrossDiffusionOperator(std::size_t begin, std::size_t end);
  /**
   * @brief Evaluate reactions and diffusion in single-thread mode.
   */
  void evaluateReactionsAndDiffusion();
  /**
   * @brief Evaluate reactions and diffusion in multi-thread mode.
   */
  void evaluateReactionsAndDiffusion_tbb();
  /**
   * @brief Replace per-voxel ``dcdt`` with spatial average.
   */
  void spatiallyAverageDcdt();
  /**
   * @brief Apply storage terms to ``dcdt`` for voxel range.
   */
  void applyStorage(std::size_t begin, std::size_t end);
  /**
   * @brief Apply storage terms to all voxels.
   */
  void applyStorage();
  /**
   * @brief Apply storage terms using multithreading.
   */
  void applyStorage_tbb();
  /**
   * @brief Forward-Euler update for voxel range.
   */
  void doForwardsEulerTimestep(double dt, std::size_t begin, std::size_t end);
  /**
   * @brief Forward-Euler update for all voxels.
   */
  void doForwardsEulerTimestep(double dt);
  /**
   * @brief Forward-Euler update using multithreading.
   */
  void doForwardsEulerTimestep_tbb(double dt);
  /**
   * @brief Clamp negative concentrations for voxel range.
   */
  void clampNegativeConcentrations(std::size_t begin, std::size_t end);
  /**
   * @brief Clamp negative concentrations for all voxels.
   */
  void clampNegativeConcentrations();
  /**
   * @brief Clamp negative concentrations using multithreading.
   */
  void clampNegativeConcentrations_tbb();
  /**
   * @brief Initialize Runge-Kutta work buffers.
   */
  void doRKInit();
  /**
   * @brief RK212 substep 1 for voxel range.
   */
  void doRK212Substep1(double dt, std::size_t begin, std::size_t end);
  /**
   * @brief RK212 substep 1 for all voxels.
   */
  void doRK212Substep1(double dt);
  /**
   * @brief RK212 substep 1 using multithreading.
   */
  void doRK212Substep1_tbb(double dt);
  /**
   * @brief RK212 substep 2 for voxel range.
   */
  void doRK212Substep2(double dt, std::size_t begin, std::size_t end);
  /**
   * @brief RK212 substep 2 for all voxels.
   */
  void doRK212Substep2(double dt);
  /**
   * @brief RK212 substep 2 using multithreading.
   */
  void doRK212Substep2_tbb(double dt);
  /**
   * @brief Generic RK substep for voxel range.
   */
  void doRKSubstep(double dt, double g1, double g2, double g3, double beta,
                   double delta, std::size_t begin, std::size_t end);
  /**
   * @brief Generic RK substep for all voxels.
   */
  void doRKSubstep(double dt, double g1, double g2, double g3, double beta,
                   double delta);
  /**
   * @brief Generic RK substep using multithreading.
   */
  void doRKSubstep_tbb(double dt, double g1, double g2, double g3, double beta,
                       double delta);
  /**
   * @brief Final RK combination for voxel range.
   */
  void doRKFinalise(double cFactor, double s2Factor, double s3Factor,
                    std::size_t begin, std::size_t end);
  /**
   * @brief Final RK combination for all voxels.
   */
  void doRKFinalise(double cFactor, double s2Factor, double s3Factor);
  /**
   * @brief Final RK combination using multithreading.
   */
  void doRKFinalise_tbb(double cFactor, double s2Factor, double s3Factor);
  /**
   * @brief Undo RK step for voxel range.
   */
  void undoRKStep(std::size_t begin, std::size_t end);
  /**
   * @brief Undo RK step for all voxels.
   */
  void undoRKStep();
  /**
   * @brief Undo RK step using multithreading.
   */
  void undoRKStep_tbb();
  /**
   * @brief Compute RK local truncation error estimate.
   */
  [[nodiscard]] PixelIntegratorError calculateRKError(double epsilon) const;
  /**
   * @brief Render RK error image and return summary text.
   */
  std::string plotRKError(common::ImageStack &images, double epsilon,
                          double max) const;
  /**
   * @brief Returns whether any species has zero storage.
   */
  [[nodiscard]] bool getHasZeroStorageSpecies() const;
  /**
   * @brief Max stable timestep for zero-storage relaxation.
   */
  [[nodiscard]] double getMaxRelaxStableTimestep() const;
  /**
   * @brief Relaxation substep 1.
   */
  void doRelaxSubstep1(double dt);
  /**
   * @brief Relaxation substep 2.
   */
  void doRelaxSubstep2(double dt);
  /**
   * @brief Undo latest relaxation step.
   */
  void undoRelaxStep();
  /**
   * @brief Compute relaxation error estimate.
   */
  [[nodiscard]] PixelIntegratorError calculateRelaxError(double epsilon) const;
  /**
   * @brief Residual for zero-storage constraints.
   */
  [[nodiscard]] PixelIntegratorError
  getZeroStorageResidual(double epsilon) const;
  /**
   * @brief Compartment id.
   */
  [[nodiscard]] const std::string &getCompartmentId() const;
  /**
   * @brief Species ids.
   */
  [[nodiscard]] const std::vector<std::string> &getSpeciesIds() const;
  /**
   * @brief Flattened concentrations.
   */
  [[nodiscard]] const std::vector<double> &getConcentrations() const;
  /**
   * @brief Set flattened concentrations.
   */
  void setConcentrations(const std::vector<double> &);
  /**
   * @brief Lower-order concentration for adaptive RK.
   */
  [[nodiscard]] double getLowerOrderConcentration(std::size_t speciesIndex,
                                                  std::size_t pixelIndex) const;
  /**
   * @brief Compartment voxel coordinates.
   */
  [[nodiscard]] const std::vector<common::Voxel> &getVoxels() const;
  /**
   * @brief Mutable derivative array.
   */
  std::vector<double> &getDcdt();
  /**
   * @brief Maximum stable timestep estimate.
   */
  [[nodiscard]] double getMaxStableTimestep() const;
};

/**
 * @brief Membrane reaction evaluator linking two compartments.
 */
class SimMembrane {
private:
  common::Symbolic sym;
  const geometry::Membrane *membrane;
  SimCompartment *compA;
  SimCompartment *compB;
  common::VolumeF voxelSize{};
  std::size_t nExtraVars{0};

public:
  /**
   * @brief Construct membrane simulation object.
   */
  SimMembrane(
      const model::Model &doc, const geometry::Membrane *membrane_ptr,
      SimCompartment *simCompA, SimCompartment *simCompB, bool doCSE = true,
      unsigned optLevel = 3, bool timeDependent = false,
      bool spaceDependent = false,
      const std::map<std::string, double, std::less<>> &substitutions = {});
  /**
   * @brief Evaluate membrane reactions and update attached compartments.
   */
  void evaluateReactions();
  /**
   * @brief Evaluate membrane reactions using conflict-free signed-face batches.
   */
  void evaluateReactions_tbb();
};

} // namespace simulate

} // namespace sme

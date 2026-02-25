#pragma once
#include "sme/optimize_options.hpp"
#include "sme/simulate_options.hpp"
#include <QRgb>
#include <cereal/cereal.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <map>
#include <string>
#include <vector>

namespace sme::model {

/**
 * @brief Mesh generation options stored in model annotation.
 */
struct MeshParameters {
  /**
   * @brief Maximum boundary points per boundary or global total (mode
   * dependent).
   */
  std::vector<std::size_t> maxPoints{};
  /**
   * @brief Maximum triangle area per compartment.
   */
  std::vector<std::size_t> maxAreas{};
  /**
   * @brief Boundary simplification algorithm selection.
   */
  std::size_t boundarySimplifierType{0};
  template <class Archive>
  void serialize(Archive &ar, std::uint32_t const version) {
    if (version == 0) {
      ar(CEREAL_NVP(maxPoints), CEREAL_NVP(maxAreas));
    } else if (version == 1) {
      ar(CEREAL_NVP(maxPoints), CEREAL_NVP(maxAreas),
         CEREAL_NVP(boundarySimplifierType));
    }
  }
};

/**
 * @brief UI display preferences persisted with the model.
 */
struct DisplayOptions {
  /**
   * @brief Per-species visibility flags.
   */
  std::vector<bool> showSpecies{};
  /**
   * @brief Display min/max concentration overlays.
   */
  bool showMinMax{true};
  /**
   * @brief Normalize colormap over all simulated timepoints.
   */
  bool normaliseOverAllTimepoints{true};
  /**
   * @brief Normalize colormap over all species.
   */
  bool normaliseOverAllSpecies{true};
  /**
   * @brief Show geometry grid overlay.
   */
  bool showGeometryGrid{false};
  /**
   * @brief Show geometry scale annotation.
   */
  bool showGeometryScale{false};
  /**
   * @brief Invert vertical image axis in views/export.
   */
  bool invertYAxis{false};

  template <class Archive>
  void serialize(Archive &ar, std::uint32_t const version) {
    if (version == 0) {
      ar(CEREAL_NVP(showSpecies), CEREAL_NVP(showMinMax),
         CEREAL_NVP(normaliseOverAllTimepoints),
         CEREAL_NVP(normaliseOverAllSpecies), CEREAL_NVP(showGeometryGrid),
         CEREAL_NVP(showGeometryScale));
    } else if (version == 1) {
      ar(CEREAL_NVP(showSpecies), CEREAL_NVP(showMinMax),
         CEREAL_NVP(normaliseOverAllTimepoints),
         CEREAL_NVP(normaliseOverAllSpecies), CEREAL_NVP(showGeometryGrid),
         CEREAL_NVP(showGeometryScale), CEREAL_NVP(invertYAxis));
    }
  }
};

/**
 * @brief Simulation settings persisted with model annotation.
 */
struct SimulationSettings {
  /**
   * @brief Sequence of simulation segments: ``(nSteps, dt)``.
   */
  std::vector<std::pair<std::size_t, double>> times{};
  /**
   * @brief Solver runtime options.
   */
  simulate::Options options{};
  /**
   * @brief Selected simulator backend.
   */
  sme::simulate::SimulatorType simulatorType{};

  template <class Archive>
  void serialize(Archive &ar, std::uint32_t const version) {
    if (version == 0) {
      ar(times, options, simulatorType);
    } else if (version == 1) {
      ar(CEREAL_NVP(times), CEREAL_NVP(options), CEREAL_NVP(simulatorType));
    }
  }
};

/**
 * @brief Complete SME model annotation payload.
 */
struct Settings {
  /**
   * @brief Simulation settings.
   */
  SimulationSettings simulationSettings{};
  /**
   * @brief Display settings.
   */
  DisplayOptions displayOptions{};
  /**
   * @brief Mesh settings.
   */
  MeshParameters meshParameters{};
  /**
   * @brief Species id to display color mapping.
   */
  std::map<std::string, QRgb> speciesColors{};
  /**
   * @brief Parameter optimization settings.
   */
  sme::simulate::OptimizeOptions optimizeOptions{};
  /**
   * @brief Imported sampled-field compartment colors.
   */
  std::vector<QRgb> sampledFieldColors{};
  /**
   * @brief Species diffusion constants for image-based definitions.
   */
  std::map<std::string, std::vector<double>> speciesDiffusionConstantArrays{};
  /**
   * @brief Species diffusion constants as analytic expressions.
   */
  std::map<std::string, std::string> speciesAnalyticDiffusionConstants{};
  /**
   * @brief Cross-diffusion expressions by species pair.
   */
  std::map<std::string, std::map<std::string, std::string>>
      speciesCrossDiffusionConstants{};
  /**
   * @brief Optional species storage coefficients.
   */
  std::map<std::string, double> speciesStorageValues{};

  template <class Archive>
  void serialize(Archive &ar, std::uint32_t const version) {
    if (version == 0) {
      ar(CEREAL_NVP(simulationSettings), CEREAL_NVP(displayOptions),
         CEREAL_NVP(meshParameters),
         ::cereal::make_nvp("speciesColours", speciesColors));
    } else if (version == 1) {
      ar(CEREAL_NVP(simulationSettings), CEREAL_NVP(displayOptions),
         CEREAL_NVP(meshParameters),
         ::cereal::make_nvp("speciesColours", speciesColors),
         CEREAL_NVP(optimizeOptions));
    } else if (version == 2) {
      ar(CEREAL_NVP(simulationSettings), CEREAL_NVP(displayOptions),
         CEREAL_NVP(meshParameters),
         ::cereal::make_nvp("speciesColours", speciesColors),
         CEREAL_NVP(optimizeOptions),
         ::cereal::make_nvp("sampledFieldColours", sampledFieldColors));
    } else if (version == 3) {
      ar(CEREAL_NVP(simulationSettings), CEREAL_NVP(displayOptions),
         CEREAL_NVP(meshParameters), CEREAL_NVP(speciesColors),
         CEREAL_NVP(optimizeOptions), CEREAL_NVP(sampledFieldColors));
    } else if (version == 4) {
      ar(CEREAL_NVP(simulationSettings), CEREAL_NVP(displayOptions),
         CEREAL_NVP(meshParameters), CEREAL_NVP(speciesColors),
         CEREAL_NVP(optimizeOptions), CEREAL_NVP(sampledFieldColors),
         CEREAL_NVP(speciesDiffusionConstantArrays),
         CEREAL_NVP(speciesAnalyticDiffusionConstants));
    } else if (version == 5) {
      ar(CEREAL_NVP(simulationSettings), CEREAL_NVP(displayOptions),
         CEREAL_NVP(meshParameters), CEREAL_NVP(speciesColors),
         CEREAL_NVP(optimizeOptions), CEREAL_NVP(sampledFieldColors),
         CEREAL_NVP(speciesDiffusionConstantArrays),
         CEREAL_NVP(speciesAnalyticDiffusionConstants),
         CEREAL_NVP(speciesStorageValues),
         CEREAL_NVP(speciesCrossDiffusionConstants));
    }
  }
};

} // namespace sme::model

CEREAL_CLASS_VERSION(sme::model::MeshParameters, 1);
CEREAL_CLASS_VERSION(sme::model::DisplayOptions, 1);
CEREAL_CLASS_VERSION(sme::model::SimulationSettings, 1);
CEREAL_CLASS_VERSION(sme::model::Settings, 5);

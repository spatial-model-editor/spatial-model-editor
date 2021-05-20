#pragma once
#include "simulate_options.hpp"
#include <QRgb>
#include <cereal/cereal.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <map>
#include <string>
#include <vector>

namespace sme::model {

struct MeshParameters {
  std::vector<std::size_t> maxPoints{};
  std::vector<std::size_t> maxAreas{};

  template <class Archive>
  void serialize(Archive &ar, std::uint32_t const version) {
    if (version == 0) {
      ar(CEREAL_NVP(maxPoints), CEREAL_NVP(maxAreas));
    }
  }
};

struct DisplayOptions {
  std::vector<bool> showSpecies{};
  bool showMinMax{true};
  bool normaliseOverAllTimepoints{true};
  bool normaliseOverAllSpecies{true};
  bool showGeometryGrid{false};
  bool showGeometryScale{false};

  template <class Archive>
  void serialize(Archive &ar, std::uint32_t const version) {
    if (version == 0) {
      ar(CEREAL_NVP(showSpecies), CEREAL_NVP(showMinMax),
         CEREAL_NVP(normaliseOverAllTimepoints),
         CEREAL_NVP(normaliseOverAllSpecies), CEREAL_NVP(showGeometryGrid),
         CEREAL_NVP(showGeometryScale));
    }
  }
};

struct SimulationSettings {
  std::vector<std::pair<std::size_t, double>> times{};
  simulate::Options options{};
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

struct Settings {
  SimulationSettings simulationSettings{};
  DisplayOptions displayOptions{};
  MeshParameters meshParameters{};
  std::map<std::string, QRgb> speciesColours{};

  template <class Archive>
  void serialize(Archive &ar, std::uint32_t const version) {
    if (version == 0) {
      ar(CEREAL_NVP(simulationSettings), CEREAL_NVP(displayOptions),
         CEREAL_NVP(meshParameters), CEREAL_NVP(speciesColours));
    }
  }
};

} // namespace sme::model

CEREAL_CLASS_VERSION(sme::model::MeshParameters, 0);
CEREAL_CLASS_VERSION(sme::model::DisplayOptions, 0);
CEREAL_CLASS_VERSION(sme::model::SimulationSettings, 1);
CEREAL_CLASS_VERSION(sme::model::Settings, 0);

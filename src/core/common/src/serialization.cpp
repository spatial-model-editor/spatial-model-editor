#include "serialization.hpp"
#include "logger.hpp"
#include <cereal/archives/binary.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/utility.hpp>
#include <fstream>

CEREAL_CLASS_VERSION(sme::utils::SmeFileContents, 1);
CEREAL_CLASS_VERSION(sme::simulate::AvgMinMax, 0);
CEREAL_CLASS_VERSION(sme::simulate::SimulationData, 0);
CEREAL_CLASS_VERSION(sme::simulate::Options, 0);
CEREAL_CLASS_VERSION(sme::simulate::DuneOptions, 0);
CEREAL_CLASS_VERSION(sme::simulate::PixelIntegratorError, 0);
CEREAL_CLASS_VERSION(sme::simulate::PixelOptions, 0);
CEREAL_CLASS_VERSION(sme::simulate::SimulationSettings, 0);

namespace sme::simulate {
template <class Archive>
void serialize(Archive &ar, sme::simulate::AvgMinMax &avgMinMax,
               std::uint32_t const version) {
  if (version == 0) {
    ar(avgMinMax.avg, avgMinMax.min, avgMinMax.max);
  }
}

template <class Archive>
void serialize(Archive &ar, sme::simulate::SimulationData &data,
               std::uint32_t const version) {
  if (version == 0) {
    ar(data.timePoints, data.concentration, data.avgMinMax,
       data.concentrationMax, data.concPadding, data.xmlModel);
  }
}

template <class Archive>
void serialize(Archive &ar, sme::simulate::Options &options,
               std::uint32_t const version) {
  if (version == 0) {
    ar(options.dune, options.pixel);
  }
}

template <class Archive>
void serialize(Archive &ar, sme::simulate::DuneOptions &options,
               std::uint32_t const version) {
  if (version == 0) {
    ar(options.discretization, options.integrator, options.dt, options.minDt,
       options.maxDt, options.increase, options.decrease, options.writeVTKfiles,
       options.newtonRelErr, options.newtonAbsErr);
  }
}

template <class Archive>
void serialize(Archive &ar, sme::simulate::PixelIntegratorError &error,
               std::uint32_t const version) {
  if (version == 0) {
    ar(error.abs, error.rel);
  }
}

template <class Archive>
void serialize(Archive &ar, sme::simulate::PixelOptions &options,
               std::uint32_t const version) {
  if (version == 0) {
    ar(options.integrator, options.maxErr, options.maxTimestep,
       options.enableMultiThreading, options.maxThreads, options.doCSE,
       options.optLevel);
  }
}

template <class Archive>
void serialize(Archive &ar, simulate::SimulationSettings &simulationSettings,
               std::uint32_t const version) {
  if (version == 0) {
    ar(simulationSettings.times, simulationSettings.options, simulationSettings.simulatorType);
  }
}

} // namespace sme::simulate

namespace sme::utils {
template <class Archive>
void serialize(Archive &ar, sme::utils::SmeFileContents &contents,
               std::uint32_t const version) {
  if (version == 0) {
    ar(contents.xmlModel, contents.simulationData);
  } else if (version == 1) {
    ar(contents.xmlModel, contents.simulationData, contents.simulationSettings);
  }
}

SmeFileContents importSmeFile(const std::string &filename){
  SmeFileContents contents{};
  std::ifstream fs(filename, std::ios::binary);
  if (fs) {
    cereal::BinaryInputArchive ar(fs);
    ar(contents);
  }
  return contents;
}

bool exportSmeFile(const std::string &filename, const SmeFileContents& contents){
  std::ofstream fs(filename, std::ios::binary);
  if (!fs) {
    return false;
  }
  cereal::BinaryOutputArchive ar{fs};
  ar(contents);
  return true;
}

} // namespace sme::utils

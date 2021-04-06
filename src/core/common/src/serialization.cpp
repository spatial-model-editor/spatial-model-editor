#include "serialization.hpp"
#include "logger.hpp"
#include <cereal/archives/binary.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <fstream>

CEREAL_CLASS_VERSION(sme::utils::SmeFileContents, 0);
CEREAL_CLASS_VERSION(sme::simulate::AvgMinMax, 0);
CEREAL_CLASS_VERSION(sme::simulate::SimulationData, 0);

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

} // namespace sme::simulate

namespace sme::utils {
template <class Archive>
void serialize(Archive &ar, sme::utils::SmeFileContents &contents,
               std::uint32_t const version) {
  if (version == 0) {
    ar(contents.xmlModel, contents.simulationData);
  }
}

SmeFile::SmeFile() = default;

SmeFile::SmeFile(const std::string &xmlModel,
                 const sme::simulate::SimulationData &simulationData)
    : contents{xmlModel, simulationData} {}

void SmeFile::setXmlModel(const std::string& xmlModel) { contents.xmlModel = xmlModel; }

const std::string &SmeFile::xmlModel() const { return contents.xmlModel; }

simulate::SimulationData &SmeFile::simulationData() {
  return contents.simulationData;
}

bool SmeFile::importFile(const std::string &filename) {
  contents = {};
  std::ifstream fs(filename, std::ios::binary);
  if(!fs){
    return false;
  }
  cereal::BinaryInputArchive ar(fs);
  ar(contents);
  if(contents.xmlModel.empty()){
    // looks like cereal archive read is a no-op if archive is invalid
    return false;
  }
  return true;
}

bool SmeFile::exportFile(const std::string &filename) const {
  std::ofstream fs(filename, std::ios::binary);
  if(!fs){
    return false;
  }
  cereal::BinaryOutputArchive ar{fs};
  ar(contents);
  return true;
}

} // namespace sme::utils

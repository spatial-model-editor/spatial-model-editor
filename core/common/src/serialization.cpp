#include "sme/serialization.hpp"
#include "sme/logger.hpp"
#include "sme/model_settings.hpp"
#include "sme/simulate_data.hpp"
#include "sme/simulate_options.hpp"
#include "sme/xml_annotation.hpp"
#include <cereal/archives/binary.hpp>
#include <cereal/archives/xml.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/memory.hpp>
#include <fstream>
#include <sbml/SBMLTransforms.h>
#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>
#include <sstream>

CEREAL_CLASS_VERSION(sme::common::SmeFileContents, 3);

namespace sme::common {

template <class Archive>
void save(Archive &ar, const sme::common::SmeFileContents &contents,
          std::uint32_t const version) {
  if (version == 3) {
    ar(contents.xmlModel, contents.simulationData);
  }
}

template <class Archive>
void load(Archive &ar, sme::common::SmeFileContents &contents,
          std::uint32_t const version) {
  if (version == 3) {
    ar(contents.xmlModel, contents.simulationData);
  } else if (version == 0 || version == 2) {
    SPDLOG_ERROR("v2");
    // simulationData wasn't wrapped in a unique_ptr until version 3
    simulate::SimulationData simulationData{};
    { ar(contents.xmlModel, simulationData); }
    contents.simulationData =
        std::make_unique<simulate::SimulationData>(std::move(simulationData));
  } else if (version == 1) {
    SPDLOG_ERROR("v1");
    // in this version, we stored the SimulationSettings in the SmeFileContents,
    // so here we need to transfer it to the annotation in the sbml xmlModel
    model::SimulationSettings simulationSettings{};
    // import settings from sme file
    // simulationData wasn't wrapped in a unique_ptr until version 3
    simulate::SimulationData simulationData{};
    { ar(contents.xmlModel, simulationData, simulationSettings); }
    contents.simulationData =
        std::make_unique<simulate::SimulationData>(std::move(simulationData));
    // import existing annotation from sbml
    std::unique_ptr<libsbml::SBMLDocument> doc(
        libsbml::readSBMLFromString(contents.xmlModel.c_str()));
    auto annotation{sme::model::getSbmlAnnotation(doc->getModel())};
    // write settings to annotation
    annotation.simulationSettings = simulationSettings;
    // write annotation back to sbml xml
    sme::model::setSbmlAnnotation(doc->getModel(), annotation);
    contents.xmlModel = libsbml::writeSBMLToStdString(doc.get());
    SPDLOG_INFO("{}", contents.xmlModel);
  }
}

std::unique_ptr<SmeFileContents> importSmeFile(const std::string &filename) {
  auto contents{std::make_unique<SmeFileContents>()};
  std::ifstream fs(filename, std::ios::binary);
  if (!fs) {
    SPDLOG_WARN("Failed to read file '{}", filename);
    return {};
  }
  try {
    cereal::BinaryInputArchive ar(fs);
    ar(*contents);
  } catch (const cereal::Exception &e) {
    // invalid/corrupted file might be identified by cereal
    SPDLOG_WARN("Failed to import file '{}'. {}", filename, e.what());
    return {};
  } catch (const std::exception &e) {
    // or it might not, but in attempting to load the invalid data cereal may
    // cause an exception to be thrown, e.g. std::length_error
    SPDLOG_WARN("Failed to import file '{}'. {}", filename, e.what());
    return {};
  }
  // other invalid inputs can result in a silent no-op
  if (contents->xmlModel.empty()) {
    SPDLOG_WARN("Failed to import file '{}'. Imported Model is empty",
                filename);
    return {};
  }
  return contents;
}

bool exportSmeFile(const std::string &filename,
                   const SmeFileContents &contents) {
  std::ofstream fs(filename, std::ios::binary);
  if (!fs) {
    return false;
  }
  cereal::BinaryOutputArchive ar{fs};
  ar(contents);
  return true;
}

std::string toXml(const model::Settings &sbmlAnnotation) {
  std::string s;
  std::locale userLocale = std::locale::global(std::locale::classic());
  std::stringstream ss;
  try {
    cereal::XMLOutputArchive ar(ss);
    ar(cereal::make_nvp("settings", sbmlAnnotation));
  } catch (const cereal::Exception &e) {
    SPDLOG_WARN("Failed to export Settings to xml: {}", e.what());
    return {};
  }
  std::vector<std::string> lines;
  for (std::string line; std::getline(ss, line);) {
    lines.push_back(line);
  }
  // trim header and footer
  // todo: do this in a less fragile way
  for (std::size_t i = 2; i + 2 < lines.size(); ++i) {
    s.append(lines[i]).append("\n");
  }
  std::locale::global(userLocale);
  return s;
}

model::Settings fromXml(const std::string &xml) {
  model::Settings sbmlAnnotation{};
  // hack until
  // https://github.com/spatial-model-editor/spatial-model-editor/issues/535 is
  // resolved: (cereal relies on strtod to read doubles and assumes C locale)
  std::locale userLocale = std::locale::global(std::locale::classic());
  // re-insert header & footer
  // todo: do this in a less fragile way
  std::string fullXml{R"(<?xml version="1.0" encoding="utf-8"?><cereal>)"};
  fullXml.append(xml);
  fullXml.append("</cereal>");
  std::stringstream ss(fullXml);
  try {
    cereal::XMLInputArchive ar(ss);
    ar(sbmlAnnotation);
  } catch (const cereal::Exception &e) {
    SPDLOG_WARN("Failed to import Settings from xml - using default values: {}",
                e.what());
    return {};
  }
  std::locale::global(userLocale);
  return sbmlAnnotation;
}

} // namespace sme::common

#include "duneini.hpp"

#include <algorithm>

#include "logger.hpp"
#include "mesh.hpp"
#include "model.hpp"
#include "pde.hpp"
#include "symbolic.hpp"
#include "tiff.hpp"
#include "utils.hpp"

static std::vector<std::string> makeValidDuneSpeciesNames(
    const std::vector<std::string> &names) {
  std::vector<std::string> duneNames = names;
  // muparser reserved words, taken from:
  // https://beltoforion.de/article.php?a=muparser&p=features
  std::vector<std::string> reservedNames{
      {"sin",  "cos",  "tan",   "asin",  "acos",  "atan", "sinh",
       "cosh", "tanh", "asinh", "acosh", "atanh", "log2", "log10",
       "log",  "ln",   "exp",   "sqrt",  "sign",  "rint", "abs",
       "min",  "max",  "sum",   "avg"}};
  // dune-copasi reserved words:
  reservedNames.insert(reservedNames.end(), {"x", "y", "t", "pi", "dim"});
  for (auto &name : duneNames) {
    SPDLOG_TRACE("name {}", name);
    std::string duneName = name;
    name = "";
    // if species name clashes with a reserved name, append an underscore
    if (std::find(reservedNames.cbegin(), reservedNames.cend(), duneName) !=
        reservedNames.cend()) {
      duneName.append("_");
    }
    // if species name clashes with another species name,
    // append another underscore
    while (std::find(duneNames.cbegin(), duneNames.cend(), duneName) !=
           duneNames.cend()) {
      duneName.append("_");
    }
    name = duneName;
    SPDLOG_TRACE("  -> {}", name);
  }
  return duneNames;
}

namespace dune {

const QString &IniFile::getText() const { return text; }

void IniFile::addSection(const QString &str) {
  if (!text.isEmpty()) {
    text.append("\n");
  }
  text.append(QString("[%1]\n").arg(str));
}

void IniFile::addSection(const QString &str1, const QString &str2) {
  addSection(QString("%1.%2").arg(str1, str2));
}

void IniFile::addSection(const QString &str1, const QString &str2,
                         const QString &str3) {
  addSection(QString("%1.%2.%3").arg(str1, str2, str3));
}

void IniFile::addValue(const QString &var, const QString &value) {
  text.append(QString("%1 = %2\n").arg(var, value));
}

void IniFile::addValue(const QString &var, int value) {
  addValue(var, QString::number(value));
}

void IniFile::addValue(const QString &var, double value, int precision) {
  addValue(var, QString::number(value, 'g', precision));
}

void IniFile::clear() { text.clear(); }

static bool compartmentContainsNonConstantSpecies(const model::Model &doc,
                                                  const QString &compID) {
  const auto &specs = doc.getSpecies().getIds(compID);
  return std::any_of(specs.cbegin(), specs.cend(), [doc = &doc](const auto &s) {
    return !doc->getSpecies().getIsConstant(s);
  });
}

DuneConverter::DuneConverter(const model::Model &SbmlDoc, double dt,
                             int doublePrecision)
    : doc(SbmlDoc) {
  double begin_time = 0.0;
  double end_time = 0.02;
  double time_step = dt;

  // grid
  ini.addSection("grid");
  ini.addValue("file", "grid.msh");
  ini.addValue("initial_level", 0);

  // simulation
  ini.addSection("model");
  ini.addValue("begin_time", begin_time, doublePrecision);
  ini.addValue("end_time", end_time, doublePrecision);
  ini.addValue("time_step", time_step, doublePrecision);
  ini.addValue("order", 1);

  // list of compartments with corresponding gmsh surface index - 1
  ini.addSection("model.compartments");
  gmshCompIndices.clear();
  int gmshCompIndex = 1;
  int duneCompIndex = 0;
  for (const auto &comp : doc.getCompartments().getIds()) {
    SPDLOG_TRACE("compartment {}", comp.toStdString());
    // skip compartments which contain no non-constant species
    if (compartmentContainsNonConstantSpecies(doc, comp)) {
      ini.addValue(comp, duneCompIndex);
      SPDLOG_TRACE("  -> added with index {}", duneCompIndex);
      gmshCompIndices.insert(gmshCompIndex);
      ++duneCompIndex;
    }
    ++gmshCompIndex;
  }
  for (const auto &mem : doc.getMembranes().getMembranes()) {
    QString compA = mem.getCompartmentA()->getId().c_str();
    QString compB = mem.getCompartmentB()->getId().c_str();
    // only add membranes which contain non-constant species
    if (compartmentContainsNonConstantSpecies(doc, compA) ||
        compartmentContainsNonConstantSpecies(doc, compB)) {
      ini.addValue(mem.getId().c_str(), duneCompIndex);
      SPDLOG_TRACE("membrane {} added with index {}", mem.getId(),
                   duneCompIndex);
      gmshCompIndices.insert(gmshCompIndex);
      ++duneCompIndex;
      independentCompartments = false;
    }
    ++gmshCompIndex;
  }

  // for each compartment
  for (const auto &compartmentID : doc.getCompartments().getIds()) {
    SPDLOG_TRACE("compartment {}", compartmentID.toStdString());
    // remove any constant species from the list of species
    std::vector<std::string> nonConstantSpecies;
    for (const auto &s : doc.getSpecies().getIds(compartmentID)) {
      if (!doc.getSpecies().getIsConstant(s)) {
        nonConstantSpecies.push_back(s.toStdString());
      }
    }
    if (!nonConstantSpecies.empty()) {
      ini.addSection("model", compartmentID);
      ini.addValue("begin_time", begin_time, doublePrecision);
      ini.addValue("end_time", end_time, doublePrecision);
      ini.addValue("time_step", time_step, doublePrecision);

      auto duneSpeciesNames = makeValidDuneSpeciesNames(nonConstantSpecies);
      for (std::size_t is = 0; is < duneSpeciesNames.size(); ++is) {
        SPDLOG_TRACE("  - species '{}' -> '{}'", nonConstantSpecies.at(is),
                     duneSpeciesNames.at(is));
      }

      // operator splitting indexing: all set to zero for now...
      ini.addSection("model", compartmentID, "operator");
      for (const auto &speciesID : duneSpeciesNames) {
        ini.addValue(speciesID.c_str(), 0);
      }

      // initial concentrations
      std::vector<QString> tiffs;
      ini.addSection("model", compartmentID, "initial");
      for (std::size_t i = 0; i < nonConstantSpecies.size(); ++i) {
        QString name = nonConstantSpecies.at(i).c_str();
        QString duneName = duneSpeciesNames.at(i).c_str();
        double initConc = doc.getSpecies().getInitialConcentration(name);
        QString expr = doc.getSpecies().getAnalyticConcentration(name);
        if (const auto *f = doc.getSpecies().getField(name);
            !f->getIsUniformConcentration()) {
          // if there is a non-uniform initial condition then make a TIFF
          auto sampledFieldName =
              QString("%1_initialConcentration").arg(duneName);
          auto sampledFieldFile = QString("%1.tif").arg(sampledFieldName);
          double max = utils::writeTIFF(
              sampledFieldFile.toStdString(),
              f->getCompartment()->getCompartmentImage().size(),
              f->getConcentration(), f->getCompartment()->getPixels(),
              doc.getGeometry().getPixelWidth());
          tiffs.push_back(sampledFieldName);
          ini.addValue(duneName,
                       QString("%2*%3(x,y)").arg(max).arg(sampledFieldName));
        } else {
          // otherwise just use initialConcentration value
          ini.addValue(duneName, initConc, doublePrecision);
        }
      }
      if (!tiffs.empty()) {
        ini.addSection("model", "data");
        for (const auto &tiff : tiffs) {
          ini.addValue(tiff, tiff + QString(".tif"));
        }
      }

      // reactions
      ini.addSection("model", compartmentID, "reaction");
      std::size_t nSpecies = nonConstantSpecies.size();

      std::vector<std::string> reacs;
      if (auto reacsInCompartment = doc.getReactions().getIds(compartmentID);
          !reacsInCompartment.isEmpty()) {
        reacs = utils::toStdString(reacsInCompartment);
      }
      pde::PDE pde(&doc, nonConstantSpecies, reacs, duneSpeciesNames);
      for (std::size_t i = 0; i < nSpecies; ++i) {
        ini.addValue(duneSpeciesNames.at(i).c_str(),
                     pde.getRHS().at(i).c_str());
      }

      // reaction term jacobian
      ini.addSection("model", compartmentID, "reaction.jacobian");
      for (std::size_t i = 0; i < nSpecies; ++i) {
        for (std::size_t j = 0; j < nSpecies; ++j) {
          QString lhs = QString("d%1__d%2")
                            .arg(duneSpeciesNames.at(i).c_str(),
                                 duneSpeciesNames.at(j).c_str());
          QString rhs = pde.getJacobian().at(i).at(j).c_str();
          ini.addValue(lhs, rhs);
        }
      }

      // diffusion coefficients
      ini.addSection("model", compartmentID, "diffusion");
      for (std::size_t i = 0; i < nSpecies; ++i) {
        ini.addValue(duneSpeciesNames.at(i).c_str(),
                     doc.getSpecies().getDiffusionConstant(
                         nonConstantSpecies[i].c_str()),
                     doublePrecision);
      }

      // output file
      ini.addSection("model", compartmentID, "writer");
      ini.addValue("file_name", compartmentID);
    }
  }

  // for each membrane do the same
  for (const auto &membrane : doc.getMembranes().getMembranes()) {
    QString membraneID = membrane.getId().c_str();
    // remove any constant species from the list of species
    std::vector<std::string> nonConstantSpecies;
    QString compA = membrane.getCompartmentA()->getId().c_str();
    QString compB = membrane.getCompartmentB()->getId().c_str();
    for (const auto &comp : {compA, compB}) {
      for (const auto &s : doc.getSpecies().getIds(comp)) {
        if (!doc.getSpecies().getIsConstant(s)) {
          nonConstantSpecies.push_back(s.toStdString());
        }
      }
    }
    if (!nonConstantSpecies.empty()) {
      ini.addSection("model", membraneID);
      ini.addValue("begin_time", begin_time, doublePrecision);
      ini.addValue("end_time", end_time, doublePrecision);
      ini.addValue("time_step", time_step, doublePrecision);

      auto duneSpeciesNames = makeValidDuneSpeciesNames(nonConstantSpecies);

      // operator splitting indexing: all set to zero for now...
      ini.addSection("model", membraneID, "operator");
      for (const auto &speciesID : duneSpeciesNames) {
        ini.addValue(speciesID.c_str(), 0);
      }

      // initial concentrations
      ini.addSection("model", membraneID, "initial");
      for (std::size_t i = 0; i < nonConstantSpecies.size(); ++i) {
        ini.addValue(duneSpeciesNames.at(i).c_str(),
                     doc.getSpecies().getInitialConcentration(
                         nonConstantSpecies.at(i).c_str()),
                     doublePrecision);
      }

      // reactions: want reactions for both neighbouring compartments
      // as well as membrane reactions (that involve species from both
      // compartments in the same reaction)
      ini.addSection("model", membraneID, "reaction");
      std::size_t nSpecies = nonConstantSpecies.size();

      std::vector<std::string> reacs;
      std::vector<std::string> reacScaleFactors;
      for (const auto &comp : {compA, compB}) {
        if (auto reacsInCompartment = doc.getReactions().getIds(comp);
            !reacsInCompartment.isEmpty()) {
          for (const auto &r : reacsInCompartment) {
            reacs.push_back(r.toStdString());
            reacScaleFactors.emplace_back("1");
          }
        }
      }
      // divide membrane reaction rates by width of membrane
      if (auto reacsInCompartment = doc.getReactions().getIds(membraneID);
          !reacsInCompartment.isEmpty()) {
        const auto &lengthUnit = SbmlDoc.getUnits().getLength();
        const auto &volumeUnit = SbmlDoc.getUnits().getVolume();
        for (const auto &r : reacsInCompartment) {
          double lengthCubedToVolFactor =
              model::pixelWidthToVolume(1.0, lengthUnit, volumeUnit);
          double width = doc.getGeometry().getMesh()->getMembraneWidth(
              membraneID.toStdString());
          double scaleFactor = width * lengthCubedToVolFactor;
          SPDLOG_INFO("  - membrane width = {} {}", width,
                      lengthUnit.symbol.toStdString());
          SPDLOG_INFO("  - [length]^3/[vol] = {}", lengthCubedToVolFactor);
          SPDLOG_INFO("  - dividing flux by {}", scaleFactor);
          reacScaleFactors.push_back(
              QString::number(scaleFactor, 'g', 17).toStdString());
          reacs.push_back(r.toStdString());
        }
      }
      pde::PDE pde(&doc, nonConstantSpecies, reacs, duneSpeciesNames,
                   reacScaleFactors);
      for (std::size_t i = 0; i < nSpecies; ++i) {
        ini.addValue(duneSpeciesNames.at(i).c_str(),
                     pde.getRHS().at(i).c_str());
      }

      // reaction term jacobian
      ini.addSection("model", membraneID, "reaction.jacobian");
      for (std::size_t i = 0; i < nSpecies; ++i) {
        for (std::size_t j = 0; j < nSpecies; ++j) {
          QString lhs = QString("d%1__d%2")
                            .arg(duneSpeciesNames.at(i).c_str(),
                                 duneSpeciesNames.at(j).c_str());
          QString rhs = pde.getJacobian().at(i).at(j).c_str();
          ini.addValue(lhs, rhs);
        }
      }

      // diffusion coefficients
      ini.addSection("model", membraneID, "diffusion");
      for (std::size_t i = 0; i < nSpecies; ++i) {
        // NOTE: setting diffusion to zero for now within membranes
        ini.addValue(duneSpeciesNames.at(i).c_str(), 0, doublePrecision);
      }

      // output file
      ini.addSection("model", membraneID, "writer");
      ini.addValue("file_name", membraneID);
    }
  }

  // logger settings
  ini.addSection("logging");
  if (SPDLOG_ACTIVE_LEVEL >= 2) {
    // for release builds disable DUNE logging
    ini.addValue("default.level", "off");
  } else {
    // for debug builds enable verbose DUNE logging
    ini.addValue("default.level", "trace");
    ini.addSection("logging.backend.model");
    ini.addValue("level", "trace");
    ini.addValue("indent", 2);

    ini.addSection("logging.backend.solver");
    ini.addValue("level", "trace");
    ini.addValue("indent", 4);
  }
}

QString DuneConverter::getIniFile() const { return ini.getText(); }

const std::unordered_set<int> &DuneConverter::getGMSHCompIndices() const {
  return gmshCompIndices;
}

bool DuneConverter::hasIndependentCompartments() const {
  return independentCompartments;
}

}  // namespace dune

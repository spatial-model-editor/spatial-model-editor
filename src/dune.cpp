#include "dune.hpp"

#include "logger.hpp"
#include "pde.hpp"
#include "reactions.hpp"
#include "symbolic.hpp"

namespace dune {

const QString &iniFile::getText() const { return text; }

void iniFile::addSection(const QString &str) {
  if (!text.isEmpty()) {
    text.append("\n");
  }
  text.append(QString("[%1]\n").arg(str));
}

void iniFile::addSection(const QString &str1, const QString &str2) {
  addSection(QString("%1.%2").arg(str1, str2));
}

void iniFile::addSection(const QString &str1, const QString &str2,
                         const QString &str3) {
  addSection(QString("%1.%2.%3").arg(str1, str2, str3));
}

void iniFile::addValue(const QString &var, const QString &value) {
  text.append(QString("%1 = %2\n").arg(var, value));
}

void iniFile::addValue(const QString &var, int value) {
  addValue(var, QString::number(value));
}

void iniFile::addValue(const QString &var, double value, int precision) {
  addValue(var, QString::number(value, 'g', precision));
}

void iniFile::clear() { text.clear(); }

DuneConverter::DuneConverter(const sbml::SbmlDocWrapper &SbmlDoc,
                             int doublePrecision)
    : doc(SbmlDoc) {
  double begin_time = 0.0;
  double end_time = 0.02;
  double time_step = 0.01;

  // grid
  ini.addSection("grid");
  ini.addValue("file", "grid.msh");
  ini.addValue("initial_level", 0);

  // simulation
  ini.addSection("model");
  ini.addValue("begin_time", begin_time, doublePrecision);
  ini.addValue("end_time", end_time, doublePrecision);
  ini.addValue("time_step", time_step, doublePrecision);

  // list of compartments with corresponding gmsh surface index - 1
  ini.addSection("model.compartments");
  for (int i = 0; i < doc.compartments.size(); ++i) {
    ini.addValue(doc.compartments[i], i);
  }

  // for each compartment
  for (const auto &compartmentID : doc.compartments) {
    const auto &speciesList = doc.species.at(compartmentID);

    ini.addSection("model", compartmentID);
    ini.addValue("begin_time", begin_time, doublePrecision);
    ini.addValue("end_time", end_time, doublePrecision);
    ini.addValue("time_step", time_step, doublePrecision);

    // remove any constant species from the list of species
    std::vector<std::string> nonConstantSpecies;
    for (const auto &s : speciesList) {
      if (!doc.getIsSpeciesConstant(s.toStdString())) {
        nonConstantSpecies.push_back(s.toStdString());
      }
    }

    // operator splitting indexing: all set to zero for now...
    ini.addSection("model", compartmentID, "operator");
    for (const auto &speciesID : nonConstantSpecies) {
      ini.addValue(speciesID.c_str(), 0);
    }

    // initial concentrations
    ini.addSection("model", compartmentID, "initial");
    std::size_t i_species = 0;
    for (const auto &speciesID : nonConstantSpecies) {
      ini.addValue(nonConstantSpecies.at(i_species).c_str(),
                   doc.getInitialConcentration(speciesID.c_str()),
                   doublePrecision);
      ++i_species;
    }

    // reactions
    ini.addSection("model", compartmentID, "reaction");
    std::size_t nSpecies = static_cast<std::size_t>(nonConstantSpecies.size());

    std::vector<std::string> reacs;
    if (doc.reactions.find(compartmentID) != doc.reactions.cend()) {
      for (const auto &r : doc.reactions.at(compartmentID)) {
        reacs.push_back(r.toStdString());
      }
    }
    pde::PDE pde(&doc, nonConstantSpecies, reacs);
    for (std::size_t i = 0; i < nSpecies; ++i) {
      ini.addValue(nonConstantSpecies.at(i).c_str(),
                   pde.getRHS().at(i).c_str());
    }

    // reaction term jacobian
    ini.addSection("model", compartmentID, "reaction.jacobian");
    for (std::size_t i = 0; i < nSpecies; ++i) {
      for (std::size_t j = 0; j < nSpecies; ++j) {
        QString lhs = QString("d%1__d%2")
                          .arg(nonConstantSpecies.at(i).c_str(),
                               nonConstantSpecies.at(j).c_str());
        QString rhs = pde.getJacobian().at(i).at(j).c_str();
        ini.addValue(lhs, rhs);
      }
    }

    // diffusion coefficients
    ini.addSection("model", compartmentID, "diffusion");
    i_species = 0;
    for (const auto &speciesID : nonConstantSpecies) {
      ini.addValue(
          nonConstantSpecies.at(i_species).c_str(),
          doc.mapSpeciesIdToField.at(speciesID.c_str()).diffusionConstant,
          doublePrecision);
      ++i_species;
    }

    // output file
    ini.addSection("model", compartmentID, "writer");
    ini.addValue("file_name", compartmentID);
  }

  // for each membrane
  for (const auto &membrane : doc.membraneVec) {
    ini.addSection("model", membrane.membraneID.c_str());
    ini.addValue("begin_time", begin_time, doublePrecision);
    ini.addValue("end_time", end_time, doublePrecision);
    ini.addValue("time_step", time_step, doublePrecision);

    // remove any constant species from the list of species
    std::vector<std::string> nonConstantSpecies;
    QString compA = membrane.compA->compartmentID.c_str();
    QString compB = membrane.compB->compartmentID.c_str();
    for (const auto &s : doc.species.at(compA)) {
      if (!doc.getIsSpeciesConstant(s.toStdString())) {
        nonConstantSpecies.push_back(s.toStdString());
      }
    }
    for (const auto &s : doc.species.at(compB)) {
      if (!doc.getIsSpeciesConstant(s.toStdString())) {
        nonConstantSpecies.push_back(s.toStdString());
      }
    }

    // operator splitting indexing: all set to zero for now...
    ini.addSection("model", membrane.membraneID.c_str(), "operator");
    for (const auto &speciesID : nonConstantSpecies) {
      ini.addValue(speciesID.c_str(), 0);
    }

    // initial concentrations
    ini.addSection("model", membrane.membraneID.c_str(), "initial");
    std::size_t i_species = 0;
    for (const auto &speciesID : nonConstantSpecies) {
      ini.addValue(nonConstantSpecies.at(i_species).c_str(),
                   doc.getInitialConcentration(speciesID.c_str()),
                   doublePrecision);
      ++i_species;
    }

    // reactions
    ini.addSection("model", membrane.membraneID.c_str(), "reaction");
    std::size_t nSpecies = static_cast<std::size_t>(nonConstantSpecies.size());

    std::vector<std::string> reacs;
    if (doc.reactions.find(membrane.membraneID.c_str()) !=
        doc.reactions.cend()) {
      for (const auto &r : doc.reactions.at(membrane.membraneID.c_str())) {
        reacs.push_back(r.toStdString());
      }
    }
    pde::PDE pde(&doc, nonConstantSpecies, reacs);
    for (std::size_t i = 0; i < nSpecies; ++i) {
      ini.addValue(nonConstantSpecies.at(i).c_str(),
                   pde.getRHS().at(i).c_str());
    }

    // reaction term jacobian
    ini.addSection("model", membrane.membraneID.c_str(), "reaction.jacobian");
    for (std::size_t i = 0; i < nSpecies; ++i) {
      for (std::size_t j = 0; j < nSpecies; ++j) {
        QString lhs = QString("d%1__d%2")
                          .arg(nonConstantSpecies.at(i).c_str(),
                               nonConstantSpecies.at(j).c_str());
        QString rhs = pde.getJacobian().at(i).at(j).c_str();
        ini.addValue(lhs, rhs);
      }
    }

    // diffusion coefficients
    ini.addSection("model", membrane.membraneID.c_str(), "diffusion");
    i_species = 0;
    for (const auto &speciesID : nonConstantSpecies) {
      ini.addValue(
          nonConstantSpecies.at(i_species).c_str(),
          doc.mapSpeciesIdToField.at(speciesID.c_str()).diffusionConstant,
          doublePrecision);
      ++i_species;
    }

    // output file
    ini.addSection("model", membrane.membraneID.c_str(), "writer");
    ini.addValue("file_name", membrane.membraneID.c_str());
  }

  // logger settings
  ini.addSection("logging");
  ini.addValue("default.level", "trace");

  ini.addSection("logging.backend.model");
  ini.addValue("level", "trace");
  ini.addValue("indent", 2);

  ini.addSection("logging.backend.solver");
  ini.addValue("level", "trace");
  ini.addValue("indent", 4);
}

QString dune::DuneConverter::getIniFile() const { return ini.getText(); }

}  // namespace dune

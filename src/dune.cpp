#include "dune.hpp"

#include "logger.hpp"
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

void iniFile::addValue(const QString &var, double value) {
  addValue(var, QString::number(value));
}

void iniFile::clear() { text.clear(); }

DuneConverter::DuneConverter(const sbml::SbmlDocWrapper &SbmlDoc)
    : doc(SbmlDoc) {
  double begin_time = 0.0;
  double end_time = 0.01;

  // standard boilerplate
  ini.addSection("logging");
  ini.addValue("default.level", "trace");
  ini.addSection("logging.backend.model");
  ini.addValue("level", "trace");
  ini.addValue("indent", 2);

  // grid
  ini.addSection("grid");
  ini.addValue("file", "grid.msh");

  // simulation
  ini.addSection("model");
  ini.addValue("begin_time", begin_time);
  ini.addValue("end_time", end_time);

  // list of compartments with corresponding gmsh surface index - 1
  ini.addSection("model.compartements");
  for (int i = 0; i < doc.compartments.size(); ++i) {
    ini.addValue(doc.compartments[i], i);
  }

  // todo: list of membranes (?)

  // for each compartment
  for (const auto &compartmentID : doc.compartments) {
    const auto &speciesList = doc.species.at(compartmentID);

    ini.addSection("model", compartmentID);
    ini.addValue("name", compartmentID);

    // remove any constant species from the list of species
    QStringList nonConstantSpecies;
    for (const auto &s : speciesList) {
      if (!doc.getIsSpeciesConstant(s.toStdString())) {
        nonConstantSpecies.push_back(s);
      }
    }

    // initial concentrations
    ini.addSection("model", compartmentID, "initial");
    int i_species = 0;
    for (const auto &speciesID : nonConstantSpecies) {
      ini.addValue(QString("u_%1").arg(QString::number(i_species)),
                   doc.getInitialConcentration(speciesID));
      ++i_species;
    }

    // reactions
    ini.addSection("model", compartmentID, "reaction");
    // reaction terms
    std::vector<std::string> uExpressions;
    std::size_t nSpecies = static_cast<std::size_t>(nonConstantSpecies.size());
    std::vector<std::string> uVars;
    for (std::size_t i = 0; i < nSpecies; ++i) {
      uVars.push_back("u_" + std::to_string(i));
    }

    if (doc.reactions.find(compartmentID) != doc.reactions.cend()) {
      reactions::Reaction reacs(&doc, nonConstantSpecies,
                                doc.reactions.at(compartmentID));
      for (std::size_t i = 0; i < nSpecies; ++i) {
        QString rhs("0.0");
        for (std::size_t j = 0; j < reacs.reacExpressions.size(); ++j) {
          // get reaction term
          QString expr = QString("%1*(%2) ")
                             .arg(QString::number(reacs.M.at(j).at(i)),
                                  reacs.reacExpressions[j].c_str());
          SPDLOG_DEBUG("Species {} Reaction {} = {}",
                       nonConstantSpecies.at(static_cast<int>(i)), j, expr);
          // parse and inline constants
          symbolic::Symbolic sym(expr.toStdString(), reacs.speciesIDs,
                                 reacs.constants[j]);
          // relabel species to u vector components
          QString newTerm = sym.relabel(reacs.speciesIDs).c_str();
          // add term to rhs
          rhs.append(QString(" + (%1)").arg(newTerm));
        }
        // reparse full rhs to simplify
        SPDLOG_DEBUG("Species {} Reparsing all reaction terms",
                     nonConstantSpecies.at(static_cast<int>(i)));
        symbolic::Symbolic sym(rhs.toStdString(), uVars, {});
        rhs = sym.simplify().c_str();
        uExpressions.push_back(rhs.toStdString());
      }
    } else {
      // no reactions
      for (std::size_t i = 0; i < nSpecies; ++i) {
        uExpressions.push_back("0.0");
      }
    }
    for (std::size_t i = 0; i < nSpecies; ++i) {
      ini.addValue(uVars[i].c_str(), uExpressions[i].c_str());
    }

    // reaction term jacobian
    ini.addSection("model", compartmentID, "reaction.jacobian");
    for (std::size_t i = 0; i < nSpecies; ++i) {
      for (std::size_t j = 0; j < nSpecies; ++j) {
        symbolic::Symbolic sym(uExpressions[i], uVars, {});
        QString lhs = QString("d(u_%1)/d(u_%2)")
                          .arg(QString::number(i), QString::number(j));
        QString rhs = sym.diff(uVars[j]).c_str();
        ini.addValue(lhs, rhs);
      }
    }

    // diffusion coefficients
    ini.addSection("model", compartmentID, "diffusion");
    i_species = 0;
    for (const auto &speciesID : nonConstantSpecies) {
      ini.addValue(QString("u_%1").arg(QString::number(i_species)),
                   doc.mapSpeciesIdToField.at(speciesID).diffusionConstant);
      ++i_species;
    }
  }
}

QString dune::DuneConverter::getIniFile() const { return ini.getText(); }

}  // namespace dune

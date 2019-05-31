#include "sbml.h"

void sbmlDocWrapper::loadFile(const std::string &filename) {
  doc.reset(libsbml::readSBMLFromFile(filename.c_str()));
  if (doc->getErrorLog()->getNumFailsWithSeverity(libsbml::LIBSBML_SEV_ERROR) >
      0) {
    // todo: Error: Invalid SBML file
    // doc->printErrors(stream)
  }

  // write raw xml to SBML panel
  xml = libsbml::writeSBMLToString(doc.get());

  model = doc->getModel();

  // get list of compartments
  species.clear();
  compartments.clear();
  for (unsigned int i = 0; i < model->getNumCompartments(); ++i) {
    const auto *comp = model->getCompartment(i);
    QString id = comp->getId().c_str();
    compartments << id;
    species[id] = QStringList();
  }

  // get all species, make a list for each compartment
  speciesIndex.clear();
  speciesID.clear();
  for (unsigned int i = 0; i < model->getNumSpecies(); ++i) {
    const auto *spec = model->getSpecies(i);
    const auto id = spec->getId().c_str();
    speciesIndex[id] = i;
    speciesID.push_back(id);
    species[spec->getCompartment().c_str()] << QString(id);
  }

  // get list of reactions
  reactions.clear();
  for (unsigned int i = 0; i < model->getNumReactions(); ++i) {
    const auto *reac = model->getReaction(i);
    reactions << QString(reac->getId().c_str());
  }

  // get list of functions
  functions.clear();
  for (unsigned int i = 0; i < model->getNumFunctionDefinitions(); ++i) {
    const auto *func = model->getFunctionDefinition(i);
    functions << QString(func->getId().c_str());
  }

  // debugging output:
  std::map<QString, QString> pde;
  // construct contribution to PDE for each species
  for (unsigned int i = 0; i < model->getNumReactions(); ++i) {
    const auto *reac = model->getReaction(i);
    QString pde_term = reac->getKineticLaw()->getFormula().c_str();
    for (unsigned i = 0; i < reac->getNumProducts(); ++i) {
      pde[reac->getProduct(i)->getSpecies().c_str()].append("+" + pde_term +
                                                            " ");
    }
    for (unsigned i = 0; i < reac->getNumReactants(); ++i) {
      pde[reac->getReactant(i)->getSpecies().c_str()].append("-" + pde_term +
                                                             " ");
    }
  }
  for (unsigned int i = 0; i < model->getNumSpecies(); ++i) {
    const auto *spec = model->getSpecies(i);
    qDebug("%s' += %s", spec->getId().c_str(),
           qPrintable(pde[spec->getId().c_str()]));
  }
}

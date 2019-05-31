#include "sbml.h"

void sbmlDocWrapper::loadFile(const std::string &filename) {
  doc.reset(libsbml::readSBMLFromFile(filename.c_str()));
  if (doc->getErrorLog()->getNumFailsWithSeverity(libsbml::LIBSBML_SEV_ERROR) >
      0) {
    isValid = false;
    // todo: doc->printErrors(stream)
  } else {
    isValid = true;
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
    if (spec->isSetHasOnlySubstanceUnits() &&
        spec->getHasOnlySubstanceUnits()) {
      // equations expect amount, not concentration for this species
      // for now this is not supported:
      qDebug() << "Error: HasOnlySubstanceUnits=true not yet supported.";
      exit(1);
    }
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
}

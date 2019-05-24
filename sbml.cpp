#include "sbml.h"

void sbmlDocWrapper::loadFile(const std::string& filename){
    doc.reset(libsbml::readSBMLFromFile(filename.c_str()));
    if (doc->getErrorLog()->getNumFailsWithSeverity(libsbml::LIBSBML_SEV_ERROR) > 0)
    {
        // todo: Error: Invalid SBML file
        // doc->printErrors(stream)
    }

    // write raw xml to SBML panel
    xml = libsbml::writeSBMLToString(doc.get());

    const auto* model = doc->getModel();

    // get list of compartments
    species.clear();
    compartments.clear();
    for(unsigned int i=0; i<model->getNumCompartments(); ++i){
        const auto* comp = model->getCompartment(i);
        QString id = comp->getId().c_str();
        compartments << id;
        species[id] = QStringList();
    }

    // get all species, make a list for each compartment
    for(unsigned int i=0; i<model->getNumSpecies(); ++i){
        const auto* spec = model->getSpecies(i);
        species[spec->getCompartment().c_str()] << QString(spec->getId().c_str());
    }

    // get list of reactions
    reactions.clear();
    for(unsigned int i=0; i<model->getNumReactions(); ++i){
        const auto* reac = model->getReaction(i);
        reactions << QString(reac->getId().c_str());
    }

    // some code that requires the libSBML spatial extension to compile:
    libsbml::SpatialPkgNamespaces sbmlns(3,1,1);
    libsbml::SBMLDocument document(&sbmlns);
}

#include "sbml.h"

void sbmlDocWrapper::loadFile(const std::string& filename){
    doc.reset(libsbml::readSBMLFromFile(filename.c_str()));
    if (doc->getErrorLog()->getNumFailsWithSeverity(libsbml::LIBSBML_SEV_ERROR) > 0)
    {
        // Error: Invalid SBML file
        // doc->printErrors(stream)
    }
    xml = libsbml::writeSBMLToString(doc.get());

    const auto* model = doc->getModel();

    reactions.clear();
    for(unsigned int i=0; i<model->getNumReactions(); ++i){
        const auto* reac = model->getReaction(i);
        const std::string& name = reac->getName();
        reactions << QString(name.c_str());
    }
    reac_model.setStringList(reactions);

    species.clear();
    for(unsigned int i=0; i<model->getNumSpecies(); ++i){
        const auto* spec = model->getSpecies(i);
        const std::string& name = spec->getId();
        species << QString(name.c_str());
    }
    spec_model.setStringList(species);

    // some code that requires the libSBML spatial extension to compile:
    libsbml::SpatialPkgNamespaces sbmlns(3,1,1);
    libsbml::SBMLDocument document(&sbmlns);
}

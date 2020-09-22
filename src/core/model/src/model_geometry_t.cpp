#include "catch_wrapper.hpp"
#include "mesh.hpp"
#include "model_compartments.hpp"
#include "model_geometry.hpp"
#include "model_membranes.hpp"
#include "model_parameters.hpp"
#include "model_reactions.hpp"
#include "model_species.hpp"
#include <QFile>
#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>

SCENARIO("Model geometry",
         "[core/model/geometry][core/model][core][model][geometry]") {
  GIVEN("no model") {
    model::ModelGeometry m;
    REQUIRE(m.getIsValid() == false);
    REQUIRE(m.getMesh() == nullptr);
    REQUIRE(m.getHasImage() == false);
    REQUIRE(m.getImage().isNull());
    m.clear();
    REQUIRE(m.getIsValid() == false);
    REQUIRE(m.getMesh() == nullptr);
    REQUIRE(m.getHasImage() == false);
    REQUIRE(m.getImage().isNull());
  }
  GIVEN("ABtoC model") {
    QFile f(":/models/ABtoC.xml");
    f.open(QIODevice::ReadOnly);
    std::unique_ptr<libsbml::SBMLDocument> doc(
        libsbml::readSBMLFromString(f.readAll().toStdString().c_str()));
    model::ModelCompartments mCompartments;
    model::ModelMembranes mMembranes;
    model::ModelGeometry mGeometry;
    model::ModelSpecies mSpecies;
    model::ModelReactions mReactions(doc->getModel(),
                                     mMembranes.getMembranes());
    mCompartments = model::ModelCompartments(
        doc->getModel(), &mGeometry, &mMembranes, &mSpecies, &mReactions);
    mGeometry =
        model::ModelGeometry(doc->getModel(), &mCompartments, &mMembranes);
    auto &m = mGeometry;
    REQUIRE(m.getIsValid() == false);
    REQUIRE(m.getMesh() == nullptr);
    REQUIRE(m.getHasImage() == false);
    REQUIRE(m.getImage().isNull());

    mGeometry.importSampledFieldGeometry(doc->getModel());
    model::ModelParameters mParameters(doc->getModel());
    mSpecies = model::ModelSpecies(doc->getModel(), &mCompartments, &mGeometry,
                                   &mParameters, &mReactions);
    REQUIRE(m.getIsValid() == true);
    REQUIRE(m.getMesh() != nullptr);
    REQUIRE(m.getHasImage() == true);
    REQUIRE(!m.getImage().isNull());
    m.clear();
    REQUIRE(m.getIsValid() == false);
    REQUIRE(m.getMesh() == nullptr);
    REQUIRE(m.getHasImage() == false);
    REQUIRE(m.getImage().isNull());
  }
}

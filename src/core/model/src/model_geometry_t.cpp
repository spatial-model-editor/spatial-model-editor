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
    WHEN("import sampled field") {
      mGeometry.importSampledFieldGeometry(doc->getModel());
      model::ModelParameters mParameters(doc->getModel());
      mSpecies = model::ModelSpecies(doc->getModel(), &mCompartments,
                                     &mGeometry, &mParameters, &mReactions);
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
    WHEN("import geometry image with alpha channel") {
      QImage img(":/test/geometry/potato_alpha_channel.png");
      QRgb bgCol = img.pixel(0, 0); // transparent background
      REQUIRE(qAlpha(bgCol) == 0);
      QRgb outerCol = img.pixel(136, 78); // opaque
      REQUIRE(qAlpha(outerCol) == 255);
      QRgb innerCol = img.pixel(176, 188); // opaque
      REQUIRE(qAlpha(innerCol) == 255);
      mGeometry.importGeometryFromImage(img);
      REQUIRE(m.getIsValid() == false);
      REQUIRE(m.getMesh() == nullptr);
      REQUIRE(m.getHasImage() == true);
      auto imgIndexed = mGeometry.getImage();
      REQUIRE(imgIndexed.colorCount() == 3);
      REQUIRE(imgIndexed.pixelColor(0, 0).alpha() == 255);
      REQUIRE(imgIndexed.pixelColor(0, 0).alpha() != qAlpha(bgCol));
      REQUIRE(imgIndexed.pixelColor(0, 0).red() == qRed(bgCol));
      REQUIRE(imgIndexed.pixelColor(0, 0).green() == qGreen(bgCol));
      REQUIRE(imgIndexed.pixelColor(0, 0).blue() == qBlue(bgCol));
      REQUIRE(imgIndexed.pixelColor(136, 78).rgba() == outerCol);
      REQUIRE(imgIndexed.pixelColor(176, 188).rgba() == innerCol);
    }
  }
}

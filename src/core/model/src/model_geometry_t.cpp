#include "catch_wrapper.hpp"
#include "mesh.hpp"
#include "model.hpp"
#include "model_geometry.hpp"
#include "serialization.hpp"
#include <QFile>
#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>

using namespace sme;

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
    model::Settings sbmlAnnotation;
    model::ModelCompartments mCompartments;
    model::ModelUnits mUnits(doc->getModel());
    model::ModelMembranes mMembranes(doc->getModel());
    model::ModelGeometry mGeometry;
    model::ModelSpecies mSpecies;
    model::ModelReactions mReactions(doc->getModel(), &mMembranes);
    common::SmeFileContents smeFileContents;
    mCompartments = model::ModelCompartments(
        doc->getModel(), &mMembranes, &mUnits, &smeFileContents.simulationData);
    mCompartments.setReactionsPtr(&mReactions);
    mCompartments.setSpeciesPtr(&mSpecies);
    mCompartments.setGeometryPtr(&mGeometry);
    mGeometry = model::ModelGeometry(doc->getModel(), &mCompartments,
                                     &mMembranes, &mUnits, &sbmlAnnotation);
    auto &m = mGeometry;
    REQUIRE(m.getIsValid() == false);
    REQUIRE(m.getMesh() == nullptr);
    REQUIRE(m.getHasImage() == false);
    REQUIRE(m.getImage().isNull());
    m.setHasUnsavedChanges(false);
    REQUIRE(m.getHasUnsavedChanges() == false);
    REQUIRE(mGeometry.getHasUnsavedChanges() == false);
    WHEN("import sampled field") {
      mGeometry.importSampledFieldGeometry(doc->getModel());
      REQUIRE(m.getHasUnsavedChanges() == true);
      REQUIRE(mGeometry.getHasUnsavedChanges() == true);
      REQUIRE(mGeometry.getPhysicalPoint({0, 0}).x() == dbl_approx(0.0));
      REQUIRE(mGeometry.getPhysicalPoint({0, 0}).y() == dbl_approx(99.0));
      REQUIRE(mGeometry.getPhysicalPointAsString({0, 0}) == "x: 0 m, y: 99 m");
      model::ModelParameters mParameters(doc->getModel());
      simulate::SimulationData data;
      mSpecies =
          model::ModelSpecies(doc->getModel(), &mCompartments, &mGeometry,
                              &mParameters, &data, &sbmlAnnotation);
      mSpecies.setReactionsPtr(&mReactions);
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
      REQUIRE(m.getHasUnsavedChanges() == false);
      REQUIRE(mGeometry.getHasUnsavedChanges() == false);
      mGeometry.importGeometryFromImage(img);
      REQUIRE(m.getHasUnsavedChanges() == true);
      REQUIRE(mGeometry.getHasUnsavedChanges() == true);
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

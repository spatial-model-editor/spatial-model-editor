#include "catch_wrapper.hpp"
#include "model_test_utils.hpp"
#include "sme/mesh.hpp"
#include "sme/model.hpp"
#include "sme/model_geometry.hpp"
#include "sme/serialization.hpp"

using namespace sme;
using namespace sme::test;

TEST_CASE("Model geometry",
          "[core/model/geometry][core/model][core][model][geometry]") {
  SECTION("no model") {
    model::ModelGeometry m;
    REQUIRE(m.getIsValid() == false);
    REQUIRE(m.getMesh() == nullptr);
    REQUIRE(m.getHasImage() == false);
    REQUIRE(m.getImages().empty());
    m.clear();
    REQUIRE(m.getIsValid() == false);
    REQUIRE(m.getMesh() == nullptr);
    REQUIRE(m.getHasImage() == false);
    REQUIRE(m.getImages().empty());
  }
  SECTION("ABtoC model") {
    auto doc{getExampleSbmlDoc(Mod::ABtoC)};
    model::Settings sbmlAnnotation;
    model::ModelCompartments mCompartments;
    model::ModelUnits mUnits(doc->getModel());
    model::ModelMembranes mMembranes(doc->getModel());
    model::ModelGeometry mGeometry;
    model::ModelSpecies mSpecies;
    model::ModelFunctions mFunctions(doc->getModel());
    model::ModelReactions mReactions(doc->getModel(), &mCompartments,
                                     &mMembranes, false);
    simulate::SimulationData simulationData{};
    mCompartments = model::ModelCompartments(doc->getModel(), &mMembranes,
                                             &mUnits, &simulationData);
    mCompartments.setReactionsPtr(&mReactions);
    mCompartments.setSpeciesPtr(&mSpecies);
    mCompartments.setGeometryPtr(&mGeometry);
    mGeometry = model::ModelGeometry(doc->getModel(), &mCompartments,
                                     &mMembranes, &mUnits, &sbmlAnnotation);
    auto &m = mGeometry;
    REQUIRE(m.getIsValid() == false);
    REQUIRE(m.getMesh() == nullptr);
    REQUIRE(m.getHasImage() == false);
    REQUIRE(m.getImages().empty());
    m.setHasUnsavedChanges(false);
    REQUIRE(m.getHasUnsavedChanges() == false);
    REQUIRE(mGeometry.getHasUnsavedChanges() == false);
    SECTION("import sampled field") {
      mGeometry.importSampledFieldGeometry(doc->getModel());
      REQUIRE(m.getHasUnsavedChanges() == true);
      REQUIRE(mGeometry.getHasUnsavedChanges() == true);
      REQUIRE(mGeometry.getPhysicalPoint({0, 0, 0}).p.x() == dbl_approx(0.0));
      REQUIRE(mGeometry.getPhysicalPoint({0, 0, 0}).p.y() == dbl_approx(99.0));
      REQUIRE(mGeometry.getPhysicalPoint({0, 0, 0}).z == dbl_approx(0.0));
      REQUIRE(mGeometry.getPhysicalPointAsString({0, 0, 0}) ==
              "x: 0 m, y: 99 m, z: 0 m");
      model::ModelParameters mParameters(doc->getModel());
      simulate::SimulationData data;
      mSpecies = model::ModelSpecies(doc->getModel(), &mCompartments,
                                     &mGeometry, &mParameters, &mFunctions,
                                     &data, &sbmlAnnotation);
      mSpecies.setReactionsPtr(&mReactions);
      REQUIRE(m.getIsValid() == true);
      REQUIRE(m.getMesh() != nullptr);
      REQUIRE(m.getHasImage() == true);
      REQUIRE(!m.getImages().empty());
      m.clear();
      REQUIRE(m.getIsValid() == false);
      REQUIRE(m.getMesh() == nullptr);
      REQUIRE(m.getHasImage() == false);
      REQUIRE(m.getImages().empty());
    }
    SECTION("import geometry image with alpha channel") {
      QImage img(":test/geometry/potato_alpha_channel.png");
      QRgb bgCol = img.pixel(0, 0); // transparent background
      REQUIRE(qAlpha(bgCol) == 0);
      QRgb outerCol = img.pixel(136, 78); // opaque
      REQUIRE(qAlpha(outerCol) == 255);
      QRgb innerCol = img.pixel(176, 188); // opaque
      REQUIRE(qAlpha(innerCol) == 255);
      REQUIRE(m.getHasUnsavedChanges() == false);
      REQUIRE(mGeometry.getHasUnsavedChanges() == false);
      mGeometry.importGeometryFromImages(img, false);
      REQUIRE(m.getHasUnsavedChanges() == true);
      REQUIRE(mGeometry.getHasUnsavedChanges() == true);
      REQUIRE(m.getIsValid() == false);
      REQUIRE(m.getMesh() == nullptr);
      REQUIRE(m.getHasImage() == true);
      auto imgIndexed = mGeometry.getImages();
      REQUIRE(imgIndexed.volume().depth() == 1);
      REQUIRE(imgIndexed[0].size() == img.size());
      REQUIRE(imgIndexed[0].colorCount() == 3);
      REQUIRE(imgIndexed[0].pixelColor(0, 0).alpha() == 255);
      REQUIRE(imgIndexed[0].pixelColor(0, 0).alpha() != qAlpha(bgCol));
      REQUIRE(imgIndexed[0].pixelColor(0, 0).red() == qRed(bgCol));
      REQUIRE(imgIndexed[0].pixelColor(0, 0).green() == qGreen(bgCol));
      REQUIRE(imgIndexed[0].pixelColor(0, 0).blue() == qBlue(bgCol));
      REQUIRE(imgIndexed[0].pixelColor(136, 78).rgba() == outerCol);
      REQUIRE(imgIndexed[0].pixelColor(176, 188).rgba() == innerCol);
    }
  }
  SECTION("very-simple-model") {
    auto m{getExampleModel(Mod::VerySimpleModel)};
    QRgb c0{qRgb(0, 2, 0)};
    QRgb c1{qRgb(144, 97, 193)};
    QRgb c2{qRgb(197, 133, 96)};
    auto cols1{m.getCompartments().getColours()};
    REQUIRE(cols1.size() == 3);
    REQUIRE(cols1[0] == c0);
    REQUIRE(cols1[1] == c1);
    REQUIRE(cols1[2] == c2);
    SECTION("import geometry & keep colour assignments") {
      QImage img(":/geometry/concave-cell-nucleus-100x100.png");
      REQUIRE(img.size() == QSize(100, 100));
      m.getGeometry().importGeometryFromImages(img, true);
      auto cols2{m.getCompartments().getColours()};
      REQUIRE(cols2.size() == 3);
      REQUIRE(cols2[0] == c0);
      REQUIRE(cols2[1] == c1);
      REQUIRE(cols2[2] == c2);
    }
    SECTION("import geometry & don't keep colour assignments") {
      QImage img(":/geometry/concave-cell-nucleus-100x100.png");
      REQUIRE(img.size() == QSize(100, 100));
      m.getGeometry().importGeometryFromImages(img, false);
      auto cols2{m.getCompartments().getColours()};
      REQUIRE(cols2.size() == 3);
      REQUIRE(cols2[0] == 0);
      REQUIRE(cols2[1] == 0);
      REQUIRE(cols2[2] == 0);
    }
    SECTION("import geometry & try to keep invalid colour assignments") {
      QImage img(":/geometry/two-blobs-100x100.png");
      REQUIRE(img.size() == QSize(100, 100));
      m.getGeometry().importGeometryFromImages(img, true);
      // new geometry image only contains c0 from previous colours
      auto cols2{m.getCompartments().getColours()};
      REQUIRE(cols2.size() == 3);
      REQUIRE(cols2[0] == c0);
      REQUIRE(cols2[1] == 0);
      REQUIRE(cols2[2] == 0);
    }
  }
}

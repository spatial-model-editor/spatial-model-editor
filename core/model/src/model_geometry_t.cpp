#include "catch_wrapper.hpp"
#include "model_test_utils.hpp"
#include "sme/mesh2d.hpp"
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
    REQUIRE(m.getMesh2d() == nullptr);
    REQUIRE(m.getMesh3d() == nullptr);
    REQUIRE(m.getIsMeshValid() == false);
    REQUIRE(m.getHasImage() == false);
    REQUIRE(m.getImages().empty());
    m.clear();
    REQUIRE(m.getIsValid() == false);
    REQUIRE(m.getMesh2d() == nullptr);
    REQUIRE(m.getMesh3d() == nullptr);
    REQUIRE(m.getIsMeshValid() == false);
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
    REQUIRE(m.getMesh2d() == nullptr);
    REQUIRE(m.getHasImage() == false);
    REQUIRE(m.getImages().empty());
    m.setHasUnsavedChanges(false);
    // https://github.com/spatial-model-editor/spatial-model-editor/issues/872
    // no geometry image but getPhysicalPoint should still not throw or segfault
    REQUIRE_NOTHROW(mGeometry.getPhysicalPoint({1, 2, 3}));
    REQUIRE(m.getHasUnsavedChanges() == false);
    REQUIRE(mGeometry.getHasUnsavedChanges() == false);
    // this is a no-op as there is no geometry image yet:
    REQUIRE_NOTHROW(m.updateGeometryImageColor(0, qRgb(255, 255, 255)));
    REQUIRE(m.getHasUnsavedChanges() == false);
    SECTION("import sampled field") {
      mGeometry.importSampledFieldGeometry(doc->getModel());
      REQUIRE(m.getHasUnsavedChanges() == true);
      REQUIRE(mGeometry.getHasUnsavedChanges() == true);
      REQUIRE(mGeometry.getPhysicalPoint({0, 0, 0}).p.x() == dbl_approx(0.5));
      REQUIRE(mGeometry.getPhysicalPoint({0, 0, 0}).p.y() == dbl_approx(99.5));
      REQUIRE(mGeometry.getPhysicalPoint({0, 0, 0}).z == dbl_approx(0.5));
      REQUIRE(mGeometry.getPhysicalPointAsString({0, 0, 0}) ==
              "x: 0.5 m, y: 99.5 m, z: 0.5 m");
      model::ModelParameters mParameters(doc->getModel());
      simulate::SimulationData data;
      mSpecies = model::ModelSpecies(doc->getModel(), &mCompartments,
                                     &mGeometry, &mParameters, &mFunctions,
                                     &data, &sbmlAnnotation);
      mSpecies.setReactionsPtr(&mReactions);
      REQUIRE(m.getIsValid() == true);
      REQUIRE(m.getIsMeshValid() == true);
      REQUIRE(m.getMesh2d() != nullptr);
      REQUIRE(m.getMesh3d() == nullptr);
      REQUIRE(m.getHasImage() == true);
      REQUIRE(!m.getImages().empty());
      m.clear();
      REQUIRE(m.getIsValid() == false);
      REQUIRE(m.getIsMeshValid() == false);
      REQUIRE(m.getMesh3d() == nullptr);
      REQUIRE(m.getMesh2d() == nullptr);
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
      mGeometry.importGeometryFromImages(common::ImageStack{{img}}, false);
      REQUIRE(m.getHasUnsavedChanges() == true);
      REQUIRE(mGeometry.getHasUnsavedChanges() == true);
      REQUIRE(m.getIsValid() == false);
      REQUIRE(m.getIsMeshValid() == false);
      REQUIRE(m.getMesh3d() == nullptr);
      REQUIRE(m.getMesh2d() == nullptr);
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
    auto cols1{m.getCompartments().getColors()};
    REQUIRE(cols1.size() == 3);
    REQUIRE(cols1[0] == c0);
    REQUIRE(cols1[1] == c1);
    REQUIRE(cols1[2] == c2);
    SECTION("update geometry image colors no-ops") {
      // invalid or no-op color changes
      REQUIRE(m.getGeometry().getHasUnsavedChanges() == false);
      // oldColor not found
      REQUIRE_NOTHROW(m.getGeometry().updateGeometryImageColor(
          qRgb(255, 255, 255), qRgb(0, 0, 0)));
      // no-op
      REQUIRE_NOTHROW(m.getGeometry().updateGeometryImageColor(c0, c0));
      REQUIRE_NOTHROW(m.getGeometry().updateGeometryImageColor(c1, c1));
      // newColor already in use
      REQUIRE_THROWS(m.getGeometry().updateGeometryImageColor(c0, c1));
      REQUIRE_THROWS(m.getGeometry().updateGeometryImageColor(c1, c0));
      // none of the above should have had any effect
      REQUIRE(m.getGeometry().getHasUnsavedChanges() == false);
    }
    SECTION("change image color and re-assign compartment (#1038)") {
      auto cols{m.getCompartments().getColors()};
      auto newCol1 = qRgb(211, 12, 77);
      REQUIRE(m.getCompartments().getColors()[1] == cols[1]);
      m.getGeometry().updateGeometryImageColor(cols[1], newCol1);
      REQUIRE(m.getCompartments().getColors()[1] == newCol1);
      auto compIds = m.getCompartments().getIds();
      m.getCompartments().setColor(compIds[0], newCol1);
      m.getCompartments().setColor(compIds[1], cols[0]);
      REQUIRE(m.getCompartments().getColors()[0] == newCol1);
      REQUIRE(m.getCompartments().getColors()[1] == cols[0]);
    }
    SECTION("update geometry image colors") {
      REQUIRE(m.getGeometry().getHasUnsavedChanges() == false);
      auto c3 = qRgb(255, 255, 255);
      auto c4 = qRgb(0, 255, 255);
      auto id = m.getCompartments().getIdFromColor(c0);
      m.getGeometry().updateGeometryImageColor(c0, c3);
      REQUIRE(m.getGeometry().getHasUnsavedChanges() == true);
      REQUIRE(m.getCompartments().getIdFromColor(c0) == "");
      REQUIRE(m.getCompartments().getIdFromColor(c3) == id);
      REQUIRE(m.getCompartments().getIdFromColor(c4) == "");
      REQUIRE(m.getGeometry().getImages().colorTable().size() == 3);
      REQUIRE(m.getCompartments().getColors().size() == 3);
      REQUIRE(m.getCompartments().getColors()[0] == c3);
      REQUIRE(m.getCompartments().getColor(id) == c3);
      m.getGeometry().updateGeometryImageColor(c3, c4);
      REQUIRE(m.getCompartments().getIdFromColor(c0) == "");
      REQUIRE(m.getCompartments().getIdFromColor(c3) == "");
      REQUIRE(m.getCompartments().getIdFromColor(c4) == id);
      REQUIRE(m.getGeometry().getHasUnsavedChanges() == true);
      REQUIRE(m.getGeometry().getImages().colorTable().size() == 3);
      REQUIRE(m.getCompartments().getColors().size() == 3);
      REQUIRE(m.getCompartments().getColor(id) == c4);
      REQUIRE(m.getCompartments().getColors()[0] == c4);
    }
    SECTION("import geometry & keep color assignments") {
      QImage img(":/geometry/concave-cell-nucleus-100x100.png");
      REQUIRE(img.size() == QSize(100, 100));
      m.getGeometry().importGeometryFromImages(common::ImageStack{{img}}, true);
      auto cols2{m.getCompartments().getColors()};
      REQUIRE(cols2.size() == 3);
      REQUIRE(cols2[0] == c0);
      REQUIRE(cols2[1] == c1);
      REQUIRE(cols2[2] == c2);
    }
    SECTION("import geometry & don't keep color assignments") {
      QImage img(":/geometry/concave-cell-nucleus-100x100.png");
      REQUIRE(img.size() == QSize(100, 100));
      m.getGeometry().importGeometryFromImages(common::ImageStack{{img}},
                                               false);
      auto cols2{m.getCompartments().getColors()};
      REQUIRE(cols2.size() == 3);
      REQUIRE(cols2[0] == 0);
      REQUIRE(cols2[1] == 0);
      REQUIRE(cols2[2] == 0);
    }
    SECTION("import geometry & try to keep invalid color assignments") {
      QImage img(":/geometry/two-blobs-100x100.png");
      REQUIRE(img.size() == QSize(100, 100));
      m.getGeometry().importGeometryFromImages(common::ImageStack{{img}}, true);
      // new geometry image only contains c0 from previous colors
      auto cols2{m.getCompartments().getColors()};
      REQUIRE(cols2.size() == 3);
      REQUIRE(cols2[0] == c0);
      REQUIRE(cols2[1] == 0);
      REQUIRE(cols2[2] == 0);
    }
  }
}

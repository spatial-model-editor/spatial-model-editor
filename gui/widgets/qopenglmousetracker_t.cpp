//
// Created by acaramizaru on 7/25/23.
//

#include "catch_wrapper.hpp"
#include "qt_test_utils.hpp"

#include "qopenglmousetracker.hpp"
#include "sme/logger.hpp"
#include <sme/mesh3d.hpp>

#include "model_test_utils.hpp"
#include "rendering.hpp"
#include "sme/image_stack.hpp"
#include "sme/tiff.hpp"
#include "sme/utils.hpp"
#include <QDir>
#include <QImage>
#include <QPoint>

using namespace sme::test;

static const char *tags{"[gui/widgets/QOpenGLMouseTracker][gui][opengl]"};

TEST_CASE("QOpenGLMouseTracker: OpenGL", tags) {

  QOpenGLMouseTracker test = QOpenGLMouseTracker();

  QColor redColor = QColor(255, 0, 0);
  QColor blueColor = QColor(0, 0, 255);
  QColor greenColor = QColor(0, 255, 0);

  QColor backgroundColor = test.getBackgroundColor();

  test.show();

  // camera position
  test.SetCameraPosition(0, 0, -70);

  SECTION("no mesh, mouse click") { sendMouseClick(&test); }
  SECTION("no mesh, mouse move") {
    sendMouseDrag(&test, QPoint(12, 12), QPoint(24, 24));
  }
  SECTION("very-simple-model, multiple set mesh calls") {
    auto model = sme::test::getExampleModel(Mod::VerySimpleModel3D);
    test.SetSubMeshes(*model.getGeometry().getMesh3d());
    test.SetSubMeshes(*model.getGeometry().getMesh3d());
    test.SetSubMeshes(*model.getGeometry().getMesh3d());
  }
  SECTION("Two disconnected eggs") {
    sme::test::createBinaryFile("geometry/3d_two_eggs_disconnected.tiff",
                                "tmp_two_eggs.tif");
    sme::common::TiffReader tiffReader(
        QDir::current().filePath("tmp_two_eggs.tif").toStdString());
    REQUIRE(tiffReader.empty() == false);
    REQUIRE(tiffReader.getErrorMessage().isEmpty());
    auto imageStack = tiffReader.getImages();
    imageStack.convertToIndexed();
    auto colours = imageStack.colorTable();
    REQUIRE(colours.size() == 3);
    std::vector<std::size_t> maxCellVolume{3};

    SECTION("All three compartments") {
      sme::common::VolumeF voxelSize(1.0, 1.0, 1.0);
      sme::common::VoxelF originPoint(0.0, 0.0, 0.0);
      sme::mesh::Mesh3d mesh3d(imageStack, maxCellVolume, voxelSize,
                               originPoint, sme::common::toStdVec(colours));
      REQUIRE(mesh3d.isValid() == true);
      REQUIRE(mesh3d.getErrorMessage().empty());
      REQUIRE(mesh3d.getTetrahedronIndices().size() == 3);

      test.SetSubMeshes(mesh3d, {redColor, blueColor, greenColor});

      // rotation around y axes and reset
      sendMouseDrag(&test, QPoint(516, 221), QPoint(546, 221));
      wait(100);
      sendMouseDrag(&test, QPoint(546, 221), QPoint(516, 221));
      wait(100);
      // rotation around x axes and reset
      sendMouseDrag(&test, QPoint(516, 221), QPoint(516, 241));
      wait(100);
      sendMouseDrag(&test, QPoint(516, 241), QPoint(516, 221));
      wait(100);

      // forcing window resize and repaint
      test.resize(500, 500);
      wait(100);
      auto QcolorSelection = QColor(test.getColour());
      // the corner initial color should be backgroundColor.
      REQUIRE(backgroundColor == QcolorSelection);

      // visibility test, disable sub-mesh
      test.setSubmeshVisibility(0, false);
      test.repaint();
      sendMouseClick(&test, {253, 235});
      QcolorSelection = QColor(test.getColour());
      REQUIRE(backgroundColor == QcolorSelection);

      // visibility test, enable sub-mesh
      test.setSubmeshVisibility(0, true);
      test.repaint();
      wait(100);
      sendMouseClick(&test, {253, 235});
      QcolorSelection = QColor(test.getColour());
      REQUIRE(backgroundColor != QcolorSelection);

      wait(100);

      // zoom
      sendMouseWheel(&test, 1);
      // move mouse over image
      sendMouseMove(&test, {10, 44});
      // click on image
      sendMouseClick(&test, {40, 40});
      // test.repaint();
      QcolorSelection = QColor(test.getColour());
      REQUIRE(blueColor != QcolorSelection);

      wait(100);

      sendMouseClick(&test, {0, 0});
      QcolorSelection = QColor(test.getColour());
      REQUIRE(redColor == QcolorSelection);

      wait(100);

      sendMouseClick(&test, {126, 319});
      QcolorSelection = QColor(test.getColour());
      REQUIRE(redColor == QcolorSelection);

      wait(100);

      // reset
      sendMouseClick(&test, {412, 445});
      QcolorSelection = QColor(test.getColour());
      REQUIRE(backgroundColor == QcolorSelection);

      wait(100);

      // reset
      sendMouseClick(&test, {412, 445});
      QcolorSelection = QColor(test.getColour());
      REQUIRE(backgroundColor == QcolorSelection);

      wait(100);

      sendMouseClick(&test, {0, 0});
      QcolorSelection = QColor(test.getColour());
      REQUIRE(redColor == QcolorSelection);

      wait(100);

      sendMouseClick(&test, {263, 253});
      wait(100);
      QcolorSelection = QColor(test.getColour());
      REQUIRE(redColor == QcolorSelection);

      wait(100);

      // reset
      sendMouseClick(&test, {412, 445});
      QcolorSelection = QColor(test.getColour());
      REQUIRE(backgroundColor == QcolorSelection);

      auto clippingPlane1 = test.BuildClippingPlane(
          QVector3D(0, 1, 0).normalized(), QVector3D(0, 10, 0), true);
      test.repaint();
      wait(100);
      sendMouseClick(&test, {259, 340});
      QcolorSelection = QColor(test.getColour());
      REQUIRE(backgroundColor == QcolorSelection);
      wait(100);

      auto clippingPlane2 = test.BuildClippingPlane(
          QVector3D(1, 0, 0).normalized(), QVector3D(0, 0, 0), true);
      REQUIRE(clippingPlane2.lock());
      test.repaint();
      wait(100);
      sendMouseClick(&test, {187, 143});
      QcolorSelection = QColor(test.getColour());
      REQUIRE(backgroundColor == QcolorSelection);
      wait(100);

      clippingPlane2.lock()->Disable();
      auto clippingPlane3 = test.BuildClippingPlane(
          QVector3D(-1, 0, 0).normalized(), QVector3D(0, 0, 0), true);
      test.repaint();
      wait(100);
      sendMouseClick(&test, {319, 144});
      QcolorSelection = QColor(test.getColour());
      REQUIRE(backgroundColor == QcolorSelection);
      wait(100);

      test.DestroyClippingPlane(clippingPlane3);
      REQUIRE_FALSE(clippingPlane3.lock());
      clippingPlane3 = test.BuildClippingPlane(QVector3D(-1, 0, 0).normalized(),
                                               QVector3D(0, 0, 0), true);
      REQUIRE(clippingPlane3.lock());
      auto clippingPlane4 = test.BuildClippingPlane(
          QVector3D(-1, 0, 0).normalized(), QVector3D(0, 0, 0), true);
      REQUIRE(clippingPlane4.lock());
      auto clippingPlane5 = test.BuildClippingPlane(
          QVector3D(-1, 0, 0).normalized(), QVector3D(0, 0, 0), true);
      REQUIRE(clippingPlane5.lock());
      auto clippingPlane6 = test.BuildClippingPlane(
          QVector3D(-1, 0, 0).normalized(), QVector3D(0, 0, 0), true);
      REQUIRE(clippingPlane6.lock());
      auto clippingPlane7 = test.BuildClippingPlane(
          QVector3D(-1, 0, 0).normalized(), QVector3D(0, 0, 0), true);
      REQUIRE(clippingPlane7.lock());
      auto clippingPlane8 = test.BuildClippingPlane(
          QVector3D(-1, 0, 0).normalized(), QVector3D(0, 0, 0), true);
      REQUIRE(clippingPlane8.lock());
      auto clippingPlane9 = test.BuildClippingPlane(
          QVector3D(-1, 0, 0).normalized(), QVector3D(0, 0, 0), true);
      REQUIRE_FALSE(clippingPlane9.lock());
      test.DestroyClippingPlane(clippingPlane8);
      REQUIRE_FALSE(clippingPlane8.lock());
      clippingPlane9 = test.BuildClippingPlane(QVector3D(-1, 0, 0).normalized(),
                                               QVector3D(0, 0, 0), true);
      REQUIRE(clippingPlane9.lock());
    }

    SECTION("All three compartments + mesh offset") {
      sme::common::VolumeF voxelSize(1.0, 1.0, 1.0);
      sme::common::VoxelF originPoint(1.0, 3.0, 3.0);
      sme::mesh::Mesh3d mesh3d(imageStack, maxCellVolume, voxelSize,
                               originPoint, sme::common::toStdVec(colours));
      REQUIRE(mesh3d.isValid() == true);
      REQUIRE(mesh3d.getErrorMessage().empty());
      REQUIRE(mesh3d.getTetrahedronIndices().size() == 3);

      test.SetSubMeshes(mesh3d, {redColor, blueColor, greenColor});

      // rotation around y axes and reset
      sendMouseDrag(&test, QPoint(516, 221), QPoint(546, 221));
      wait(100);
      sendMouseDrag(&test, QPoint(546, 221), QPoint(516, 221));
      wait(100);
      // rotation around x axes and reset
      sendMouseDrag(&test, QPoint(516, 221), QPoint(516, 241));
      wait(100);
      sendMouseDrag(&test, QPoint(516, 241), QPoint(516, 221));
      wait(100);

      // forcing window resize and repaint
      test.resize(500, 500);
      wait(100);
      auto QcolorSelection = QColor(test.getColour());
      // the corner initial color should be backgroundColor.
      REQUIRE(backgroundColor == QcolorSelection);

      // visibility test, disable sub-mesh
      test.setSubmeshVisibility(0, false);
      test.repaint();
      sendMouseClick(&test, {253, 235});
      QcolorSelection = QColor(test.getColour());
      REQUIRE(backgroundColor == QcolorSelection);

      wait(100);
      // visibility test, enable sub-mesh
      test.setSubmeshVisibility(0, true);
      test.repaint();
      sendMouseClick(&test, {253, 235});
      QcolorSelection = QColor(test.getColour());
      REQUIRE(backgroundColor != QcolorSelection);

      wait(100);

      // zoom
      sendMouseWheel(&test, 1);
      // move mouse over image
      sendMouseMove(&test, {10, 44});
      // click on image
      sendMouseClick(&test, {40, 40});
      // test.repaint();
      QcolorSelection = QColor(test.getColour());
      REQUIRE(blueColor != QcolorSelection);

      wait(100);

      sendMouseClick(&test, {0, 0});
      QcolorSelection = QColor(test.getColour());
      REQUIRE(redColor == QcolorSelection);

      wait(100);

      sendMouseClick(&test, {126, 319});
      QcolorSelection = QColor(test.getColour());
      REQUIRE(redColor == QcolorSelection);

      wait(100);

      // reset
      sendMouseClick(&test, {412, 445});
      QcolorSelection = QColor(test.getColour());
      REQUIRE(backgroundColor == QcolorSelection);

      wait(100);

      // reset
      sendMouseClick(&test, {412, 445});
      QcolorSelection = QColor(test.getColour());
      REQUIRE(backgroundColor == QcolorSelection);

      wait(100);

      sendMouseClick(&test, {0, 0});
      QcolorSelection = QColor(test.getColour());
      REQUIRE(redColor == QcolorSelection);

      wait(100);

      sendMouseClick(&test, {331, 322});
      QcolorSelection = QColor(test.getColour());
      REQUIRE(redColor == QcolorSelection);

      wait(100);

      // reset
      sendMouseClick(&test, {412, 445});
      QcolorSelection = QColor(test.getColour());
      REQUIRE(backgroundColor == QcolorSelection);

      auto clippingPlane1 = test.BuildClippingPlane(
          QVector3D(0, 1, 0).normalized(), QVector3D(0, 10, 0), true);
      test.repaint();
      wait(100);
      sendMouseClick(&test, {259, 340});
      QcolorSelection = QColor(test.getColour());
      REQUIRE(backgroundColor == QcolorSelection);
      wait(100);

      auto clippingPlane2 = test.BuildClippingPlane(
          QVector3D(1, 0, 0).normalized(), QVector3D(0, 0, 0), true);
      REQUIRE(clippingPlane2.lock());
      test.repaint();
      wait(100);
      sendMouseClick(&test, {187, 143});
      QcolorSelection = QColor(test.getColour());
      REQUIRE(backgroundColor == QcolorSelection);
      wait(100);

      clippingPlane2.lock()->Disable();
      auto clippingPlane3 = test.BuildClippingPlane(
          QVector3D(-1, 0, 0).normalized(), QVector3D(0, 0, 0), true);
      REQUIRE(clippingPlane3.lock());
      test.repaint();
      wait(100);
      sendMouseClick(&test, {319, 144});
      QcolorSelection = QColor(test.getColour());
      REQUIRE(backgroundColor == QcolorSelection);
      wait(100);
      clippingPlane3.lock()->TranslateAlongsideNormal(10);
      test.repaint();
      wait(100);
      clippingPlane3.lock()->TranslateAlongsideNormal(-20);
      test.repaint();
      wait(100);

      test.DestroyClippingPlane(clippingPlane3);
      REQUIRE_FALSE(clippingPlane3.lock());
      clippingPlane3 = test.BuildClippingPlane(QVector3D(-1, 0, 0).normalized(),
                                               QVector3D(0, 0, 0), true);
      REQUIRE(clippingPlane3.lock());
      auto clippingPlane4 = test.BuildClippingPlane(
          QVector3D(-1, 0, 0).normalized(), QVector3D(0, 0, 0), true);
      REQUIRE(clippingPlane4.lock());
      auto clippingPlane5 = test.BuildClippingPlane(
          QVector3D(-1, 0, 0).normalized(), QVector3D(0, 0, 0), true);
      REQUIRE(clippingPlane5.lock());
      auto clippingPlane6 = test.BuildClippingPlane(
          QVector3D(-1, 0, 0).normalized(), QVector3D(0, 0, 0), true);
      REQUIRE(clippingPlane6.lock());
      auto clippingPlane7 = test.BuildClippingPlane(
          QVector3D(-1, 0, 0).normalized(), QVector3D(0, 0, 0), true);
      REQUIRE(clippingPlane7.lock());
      auto clippingPlane8 = test.BuildClippingPlane(
          QVector3D(-1, 0, 0).normalized(), QVector3D(0, 0, 0), true);
      REQUIRE(clippingPlane8.lock());
      auto clippingPlane9 = test.BuildClippingPlane(
          QVector3D(-1, 0, 0).normalized(), QVector3D(0, 0, 0), true);
      REQUIRE_FALSE(clippingPlane9.lock());
      test.DestroyClippingPlane(clippingPlane8);
      REQUIRE_FALSE(clippingPlane8.lock());
      clippingPlane9 = test.BuildClippingPlane(QVector3D(-1, 0, 0).normalized(),
                                               QVector3D(0, 0, 0), true);
      REQUIRE(clippingPlane9.lock());

      auto planes = test.GetAllClippingPlanes();

      for (const auto &var : planes) {
        auto elem =
            std::dynamic_pointer_cast<rendering::ClippingPlane>(var.lock());

        // if (elem->name == "Camera Plane")
        {
          elem->TranslateAlongsideNormal(1000);
          break;
        }
      }

      wait(100);
      sendMouseClick(&test, {259, 340});
      QcolorSelection = QColor(test.getColour());
      REQUIRE(backgroundColor == QcolorSelection);
    }
  }
}

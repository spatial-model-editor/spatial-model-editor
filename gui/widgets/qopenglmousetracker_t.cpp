//
// Created by acaramizaru on 7/25/23.
//

#include "catch_wrapper.hpp"
#include "qt_test_utils.hpp"

#include "qopenglmousetracker.hpp"
#include "sme/logger.hpp"
#include <sme/mesh3d.hpp>

#include "model_test_utils.hpp"
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
  QColor blackColor = QColor(0, 0, 0);
  QColor greenColor = QColor(0, 255, 0);

  test.show();
  //  wait(100);

  // camera position
  test.SetCameraPosition(20, 20, -50);

  SECTION("Two disconnected eggs") {
    sme::test::createBinaryFile("geometry/3d_two_eggs_disconnected.tiff",
                                "tmp_two_eggs.tif");
    sme::common::TiffReader tiffReader(
        QDir::current().filePath("tmp_two_eggs.tif").toStdString());
    REQUIRE(tiffReader.empty() == false);
    REQUIRE(tiffReader.getErrorMessage().isEmpty());
    auto imageStack = tiffReader.getImages();
    imageStack.convertToIndexed();
    sme::common::VolumeF voxelSize(1.0, 1.0, 1.0);
    sme::common::VoxelF originPoint(0.0, 0.0, 0.0);
    auto colours = imageStack[0].colorTable();
    REQUIRE(colours.size() == 3);
    QRgb colOutside{0xff000000};
    QRgb colCell{0xff7f7f7f};
    QRgb colNucleus{0xffffffff};
    std::vector<std::size_t> maxCellVolume{3};

    SECTION("All three compartments") {
      sme::mesh::Mesh3d mesh3d(imageStack, maxCellVolume, voxelSize,
                               originPoint, sme::common::toStdVec(colours));
      REQUIRE(mesh3d.isValid() == true);
      REQUIRE(mesh3d.getErrorMessage().empty());
      REQUIRE(mesh3d.getTetrahedronIndices().size() == 3);

      test.SetSubMeshes(mesh3d, {redColor, blueColor, greenColor});

      wait(100);

      auto QcolorSelection = QColor(test.getColour());

      // forced windows resize and forced repainting
      test.resize(500, 500);
      // test.repaint();

      // the corner initial color should be black.
      REQUIRE(blackColor == QcolorSelection);

      // visibility test
      test.setSubmeshVisibility(0, false);
      test.repaint();
      sendMouseClick(&test, {253, 235});
      QcolorSelection = QColor(test.getColour());
      REQUIRE(blackColor == QcolorSelection);

      wait(100);

      test.setSubmeshVisibility(0, true);
      test.repaint();
      sendMouseClick(&test, {253, 235});
      QcolorSelection = QColor(test.getColour());
      REQUIRE(blackColor != QcolorSelection);

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

      REQUIRE(greenColor == QcolorSelection);

      wait(100);

      // reset
      sendMouseClick(&test, {412, 445});

      QcolorSelection = QColor(test.getColour());

      REQUIRE(blackColor == QcolorSelection);

      wait(100);

      // reset
      sendMouseClick(&test, {412, 445});

      QcolorSelection = QColor(test.getColour());

      REQUIRE(blackColor == QcolorSelection);

      wait(100);

      sendMouseClick(&test, {0, 0});

      QcolorSelection = QColor(test.getColour());

      REQUIRE(redColor == QcolorSelection);

      wait(100);

      sendMouseClick(&test, {331, 322});

      QcolorSelection = QColor(test.getColour());

      REQUIRE(greenColor == QcolorSelection);

      wait(100);

      // reset
      sendMouseClick(&test, {412, 445});

      QcolorSelection = QColor(test.getColour());

      REQUIRE(blackColor == QcolorSelection);
    }
  }
}

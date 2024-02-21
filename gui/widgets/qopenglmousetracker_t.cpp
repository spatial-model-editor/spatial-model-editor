//
// Created by acaramizaru on 7/25/23.
//

#include "catch_wrapper.hpp"
#include "qt_test_utils.hpp"

#include "qopenglmousetracker.hpp"
#include "rendering/rendering.hpp"
#include "sme/logger.hpp"
#include <sme/mesh3d.hpp>

#include "model_test_utils.hpp"
#include "sme/image_stack.hpp"
#include "sme/tiff.hpp"
#include "sme/utils.hpp"
#include <QDir>
#include <QFile>
#include <QImage>
#include <QPoint>

using namespace sme::test;

static const char *tags{"[gui/widgets/QOpenGLMouseTracker][gui][opengl]"};

TEST_CASE("QOpenGLMouseTracker: OpenGL", tags) {

  //  SECTION("CGAL mesh3d from image, with sme input") {
  //
  //    QOpenGLMouseTracker test = QOpenGLMouseTracker();
  //
  //    QColor redColor = QColor(255, 0, 0);
  //    QColor blueColor = QColor(0, 0, 255);
  //    QColor blackColor = QColor(0, 0, 0);
  //
  //    test.show();
  //
  //    wait(100);
  //
  //    // camera position
  //    test.SetCameraPosition(0, 0, -10);
  // editor
  //    auto model = sme::model::Model();
  //    model.importFile("/home/hcaramizaru/Dropbox/Work-shared/Heidelberg-IWR/"
  //                     "spatial-model-editor-resources/cell-3d-model.sme");
  //    REQUIRE(model.getIsValid());
  //
  //    sme::mesh::Mesh3d testMesh(model);
  //
  //    REQUIRE(testMesh.getNumberOfSubMeshes() == 5);
  //
  //    rendering::SMesh sphereMesh = testMesh.GetMesh(1);
  //    test.addMesh(sphereMesh, redColor);
  //
  //    wait(100000);
  //  }

  QOpenGLMouseTracker test = QOpenGLMouseTracker();

  QColor redColor = QColor(255, 0, 0);
  QColor blueColor = QColor(0, 0, 255);
  // QColor blackColor = QColor(0, 0, 0);
  QColor greenColor = QColor(0, 255, 0);

  test.show();

  wait(100);

  // camera position
  test.SetCameraPosition(20, 10, -10);

  SECTION("Two disconnected eggs") {
    sme::test::createBinaryFile("geometry/3d_two_eggs_disconnected.tiff",
                                "tmp_two_eggs.tif");
    sme::common::TiffReader tiffReader(
        QDir::current().filePath("tmp_two_eggs.tif").toStdString());
    REQUIRE(tiffReader.empty() == false);
    REQUIRE(tiffReader.getErrorMessage().isEmpty());
    auto imageStack = tiffReader.getImages();
    REQUIRE(imageStack.volume().width() == 40);
    REQUIRE(imageStack.volume().height() == 40);
    REQUIRE(imageStack.volume().depth() == 40);
    imageStack.convertToIndexed();
    sme::common::VolumeF voxelSize(1.0, 1.0, 1.0);
    sme::common::VoxelF originPoint(0.0, 0.0, 0.0);
    auto colours = imageStack[0].colorTable();
    REQUIRE(colours.size() == 3);
    QRgb colOutside{0xff000000};
    QRgb colCell{0xff7f7f7f};
    QRgb colNucleus{0xffffffff};
    std::vector<std::size_t> maxCellVolume{3};
    //    SECTION("No compartment colours") {
    //      sme::mesh::Mesh3d mesh3d(imageStack, maxCellVolume, voxelSize,
    //      originPoint,
    //                          {});
    //      REQUIRE(mesh3d.isValid() == false);
    //    }
    //    SECTION("Outside only") {
    //      sme::mesh::Mesh3d mesh3d(imageStack, maxCellVolume, voxelSize,
    //      originPoint,
    //                          {colOutside});
    //      REQUIRE(mesh3d.isValid() == true);
    //      REQUIRE(mesh3d.getErrorMessage().empty());
    //      REQUIRE(mesh3d.getTetrahedronIndices().size() == 1);
    //
    //      test.SetSubMeshes(mesh3d, {redColor});
    //
    //      wait(100);
    //
    //    }
    //    SECTION("Cell only") {
    //      sme::mesh::Mesh3d mesh3d(imageStack, maxCellVolume, voxelSize,
    //      originPoint,
    //                          {colCell});
    //      REQUIRE(mesh3d.isValid() == true);
    //      REQUIRE(mesh3d.getErrorMessage().empty());
    //      REQUIRE(mesh3d.getTetrahedronIndices().size() == 1);
    //    }
    //    SECTION("Nucleus only") {
    //      sme::mesh::Mesh3d mesh3d(imageStack, maxCellVolume, voxelSize,
    //      originPoint,
    //                          {colNucleus});
    //      REQUIRE(mesh3d.isValid() == true);
    //      REQUIRE(mesh3d.getErrorMessage().empty());
    //      REQUIRE(mesh3d.getTetrahedronIndices().size() == 1);
    //    }
    //    SECTION("Nucleus+Cell only") {
    //      sme::mesh::Mesh3d mesh3d(imageStack, maxCellVolume, voxelSize,
    //      originPoint,
    //                          {colNucleus, colCell});
    //      REQUIRE(mesh3d.isValid() == true);
    //      REQUIRE(mesh3d.getErrorMessage().empty());
    //      REQUIRE(mesh3d.getTetrahedronIndices().size() == 2);
    //    }
    //    SECTION("Nucleus+Outside only") {
    //      sme::mesh::Mesh3d mesh3d(imageStack, maxCellVolume, voxelSize,
    //      originPoint,
    //                          {colNucleus, colOutside});
    //      REQUIRE(mesh3d.isValid() == true);
    //      REQUIRE(mesh3d.getErrorMessage().empty());
    //      REQUIRE(mesh3d.getTetrahedronIndices().size() == 2);
    //    }
    //    SECTION("Cell+Outside only") {
    //      sme::mesh::Mesh3d mesh3d(imageStack, maxCellVolume, voxelSize,
    //      originPoint,
    //                          {colOutside, colCell});
    //      REQUIRE(mesh3d.isValid() == true);
    //      REQUIRE(mesh3d.getErrorMessage().empty());
    //      REQUIRE(mesh3d.getTetrahedronIndices().size() == 2);
    //    }
    SECTION("All three compartments") {
      sme::mesh::Mesh3d mesh3d(imageStack, maxCellVolume, voxelSize,
                               originPoint, sme::common::toStdVec(colours));
      REQUIRE(mesh3d.isValid() == true);
      REQUIRE(mesh3d.getErrorMessage().empty());
      REQUIRE(mesh3d.getTetrahedronIndices().size() == 3);

      std::vector<QColor> colors_{QColor(colours[0]), QColor(colours[1]),
                                  QColor(colours[2])};

      test.SetSubMeshes(mesh3d, {redColor, blueColor, greenColor});

      wait(1000000);

      //      QFile f("grid.msh");
      //      f.open(QIODevice::ReadWrite | QIODevice::Truncate |
      //      QIODevice::Text); f.write(mesh3d.getGMSH().toUtf8());
    }
  }

  return;

  //  // loading meshes
  //  QFile::copy(":/test/rendering/Objects/sphere.ply", "tmp_sphere.ply");
  //  REQUIRE(QFile::exists("tmp_sphere.ply"));
  //  rendering::SMesh sphereMesh = rendering::ObjectLoader::LoadMesh(
  //      QDir::current().filePath("tmp_sphere.ply").toStdString());
  //  //  test.addMesh(sphereMesh, redColor);
  ////  test.SetSubMeshes(sphereMesh, {redColor});
  //
  //  wait(100);
  //
  //  QFile::copy(":/test/rendering/Objects/teapot.ply", "tmp_teapot.ply");
  //  QFileInfo info("tmp_teapot.ply");
  //  REQUIRE(QFile::exists("tmp_teapot.ply"));
  //
  //  wait(100);
  //
  //  rendering::SMesh teapotMesh = rendering::ObjectLoader::LoadMesh(
  //      QDir::current().filePath("tmp_teapot.ply").toStdString());
  //  //  test.addMesh(teapotMesh, blueColor);
  //  test.SetSubMeshes(teapotMesh, {blueColor});
  //
  //  auto QcolorSelection = QColor(test.getColour());
  //
  //  // forced windows resize and forced repainting
  //  test.resize(500, 500);
  //  // test.repaint();
  //
  //  // the corner initial color should be black.
  //  REQUIRE(blackColor == QcolorSelection);
  //
  //  // zoom
  //  sendMouseWheel(&test, 1);
  //
  //  // move mouse over image
  //  sendMouseMove(&test, {10, 44});
  //
  //  // click on image
  //  sendMouseClick(&test, {40, 40});
  //  // test.repaint();
  //
  //  QcolorSelection = QColor(test.getColour());
  //
  //  REQUIRE(blueColor != QcolorSelection);
  //
  //  wait(100);
  //
  //  sendMouseClick(&test, {0, 0});
  //
  //  QcolorSelection = QColor(test.getColour());
  //
  //  REQUIRE(blueColor == QcolorSelection);
  //
  //  wait(100);
  //
  //  sendMouseClick(&test, {376, 366});
  //
  //  QcolorSelection = QColor(test.getColour());
  //
  //  REQUIRE(redColor == QcolorSelection);
  //
  //  wait(100);
  // gui
  //  // reset
  //  sendMouseClick(&test, {412, 445});
  //
  //  QcolorSelection = QColor(test.getColour());
  //
  //  REQUIRE(blackColor == QcolorSelection);
  //
  //  wait(100);
  //
  //  // reset
  //  sendMouseClick(&test, {412, 445});
  //
  //  QcolorSelection = QColor(test.getColour());
  //
  //  REQUIRE(blackColor == QcolorSelection);
  //
  //  wait(100);
  //
  //  sendMouseClick(&test, {0, 0});
  //
  //  QcolorSelection = QColor(test.getColour());
  //
  //  REQUIRE(blueColor == QcolorSelection);
  //
  //  wait(100);
  //
  //  sendMouseClick(&test, {376, 366});
  //
  //  QcolorSelection = QColor(test.getColour());
  //
  //  REQUIRE(redColor == QcolorSelection);
  //
  //  wait(100);
  //
  //  // reset
  //  sendMouseClick(&test, {412, 445});
  //
  //  QcolorSelection = QColor(test.getColour());
  //
  //  REQUIRE(blackColor == QcolorSelection);
}

//
// Created by acaramizaru on 7/25/23.
//

#include "catch_wrapper.hpp"
#include "qt_test_utils.hpp"

#include "rendering/rendering.hpp"

using namespace sme::test;

#include "qopenglmousetracker.h"

static const char *tags{"[gui/widgets/QOpenGLMouseTracker][gui/widgets][gui]"};

TEST_CASE("QOpenGLMouseTracker: OpenGL", tags) {

  QOpenGLMouseTracker *test = new QOpenGLMouseTracker();
  REQUIRE(test != nullptr);

  // default buffers structure
  QSurfaceFormat format;
  format.setDepthBufferSize(24);
  format.setStencilBufferSize(8);
  format.setProfile(QSurfaceFormat::CoreProfile);
  QSurfaceFormat::setDefaultFormat(format);

  rendering::Vector4 redColor = rendering::Vector4(1.0f, 0.0f, 0.0f);
  rendering::Vector4 blueColor = rendering::Vector4(0.0f, 0.0f, 1.0f);
  rendering::Vector4 blackColor = rendering::Vector4();

  test->show();

  // camera position
  test->SetCameraPosition(0, 0, -10);

  // loading meshes

  QFile::copy(":/test/rendering/Objects/sphere.ply", "tmp_sphere.ply");
  REQUIRE(QFile::exists("tmp_sphere.ply"));
  rendering::SMesh sphereMesh = rendering::ObjectLoader::LoadMesh(
      QDir::current().filePath("tmp_sphere.ply").toStdString());
  test->addMesh(sphereMesh, redColor);

  QFile::copy(":/test/rendering/Objects/teapot.ply", "tmp_teapot.ply");
  REQUIRE(QFile::exists("tmp_teapot.ply"));
  rendering::SMesh teapotMesh = rendering::ObjectLoader::LoadMesh(
      QDir::current().filePath("tmp_teapot.ply").toStdString());
  test->addMesh(teapotMesh, blueColor);

  auto QcolorSelection = QColor(test->getColour());

  // forced windows resize and forced repainting
  test->resize(100, 100);
  test->repaint();

  // the corner initial color should be black.
  REQUIRE(blackColor == rendering::Vector4(QcolorSelection.redF(),
                                           QcolorSelection.greenF(),
                                           QcolorSelection.blueF()));
  // zoom
  sendMouseWheel(test, 1);
  // move mouse over image
  sendMouseMove(test, {10, 44});
  // click on image
  sendMouseClick(test, {40, 40});
  test->repaint();
  sendMouseClick(test, {40, 40});

  QcolorSelection = QColor(test->getColour());

  rendering::Vector4 colorSelect = rendering::Vector4(QcolorSelection.redF(),
                     QcolorSelection.greenF(),
                     QcolorSelection.blueF());

  REQUIRE(blueColor == colorSelect);

  // wait(50000);
  wait();
}

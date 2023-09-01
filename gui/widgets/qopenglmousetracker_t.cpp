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

  QSurfaceFormat format;
  format.setDepthBufferSize(24);
  format.setStencilBufferSize(8);

  format.setProfile(QSurfaceFormat::CoreProfile);

  QSurfaceFormat::setDefaultFormat(format);


  rendering::Vector4 redColor = rendering::Vector4(1.0f, 0.0f, 0.0f);
  rendering::Vector4 blueColor = rendering::Vector4(0.0f, 0.0f, 1.0f);

  test->show();

  test->SetCameraPosition(0,0,-10);

  QFile::copy(":/test/rendering/Objects/sphere.ply", "tmp_sphere.ply");

  rendering::SMesh sphereMesh = rendering::ObjectLoader::LoadMesh(QDir::current().filePath("tmp_sphere.ply").toStdString());
  test->addMesh(sphereMesh, redColor);

  QFile::copy(":/test/rendering/Objects/teapot.ply", "tmp_teapot.ply");

  rendering::SMesh teapotMesh = rendering::ObjectLoader::LoadMesh(QDir::current().filePath("tmp_teapot.ply").toStdString());
  test->addMesh(teapotMesh, blueColor);

  wait(50000);
}

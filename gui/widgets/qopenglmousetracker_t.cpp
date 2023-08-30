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


  Vector4 redColor = Vector4(1.0f, 0.0f, 0.0f);
  Vector4 blueColor = Vector4(0.0f, 0.0f, 1.0f);

  test->show();

  test->SetCameraPosition(0,0,-10);

  SMesh sphereMesh = ObjectLoader::LoadMesh("/home/acaramizaru/git/spatial-model-editor/gui/rendering/Objects/sphere.ply");
  test->addMesh(sphereMesh, redColor);

  SMesh teapotMesh = ObjectLoader::LoadMesh("/home/acaramizaru/git/spatial-model-editor/gui/rendering/Objects/teapot.ply");
  test->addMesh(teapotMesh, blueColor);

  wait(50000);
}

//
// Created by acaramizaru on 6/29/23.
//

#include "catch_wrapper.hpp"
#include "qt_test_utils.hpp"

#include "rendering/rendering.hpp"

using namespace sme::test;

#include "qopengltriangle.hpp"

static const char *tags{"[gui/widgets/QOpenGLTriangle][gui/widgets][gui]"};

TEST_CASE("QOpenGLTriangle: OpenGL", tags) {

  QOpenGLTriangle *test = new QOpenGLTriangle();

  QSurfaceFormat format;
  format.setDepthBufferSize(24);
  format.setStencilBufferSize(8);
  format.setVersion(3, 2);
  format.setProfile(QSurfaceFormat::CoreProfile);
  QSurfaceFormat::setDefaultFormat(format);

  test->show();
  wait(50000);
}

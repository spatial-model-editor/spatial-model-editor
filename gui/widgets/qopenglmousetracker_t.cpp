//
// Created by acaramizaru on 6/29/23.
//

#include "catch_wrapper.hpp"
#include "qt_test_utils.hpp"
#include <QApplication>

using namespace sme::test;


#include "qopenglmousetracker.hpp"

static const char *tags{"[gui/widgets/QOpenGLMouseTracker][gui/widgets][gui]"};

TEST_CASE("QOpenGLMouseTracker: OpenGL", tags) {

  QOpenGLMouseTracker *test = new QOpenGLMouseTracker();

  QSurfaceFormat format;
  format.setDepthBufferSize(24);
  format.setStencilBufferSize(8);
  format.setVersion(3, 2);
  format.setProfile(QSurfaceFormat::CoreProfile);
  QSurfaceFormat::setDefaultFormat(format);

  test->show();
  wait(50000);
}
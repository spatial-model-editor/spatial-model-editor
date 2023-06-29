//
// Created by acaramizaru on 6/29/23.
//

#include "catch_wrapper.hpp"
#include "qt_test_utils.hpp"

using namespace sme::test;


#include "QOpenGLMouseTracker.hpp"

static const char *tags{"[gui/widgets/QOpenGLMouseTracker][gui/widgets][gui]"};

TEST_CASE("QOpenGLMouseTracker: OpenGL", tags) {

  QOpenGLMouseTracker *test = new QOpenGLMouseTracker();

  test->show();

}

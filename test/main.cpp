#include "catch_wrapper.hpp"
#include "sme/logger.hpp"
#include <QApplication>
#include <QSurfaceFormat>
#include <catch2/catch_session.hpp>

int main(int argc, char *argv[]) {
  Catch::StringMaker<double>::precision = 25;
  Catch::StringMaker<float>::precision = 25;

#ifdef SME_ENABLE_GUI_TESTS
  QSurfaceFormat format;
  // This comment is a reminder for whenever we can test using a Mac
  // machine.
  //  format.setProfile(QSurfaceFormat::CoreProfile);
  format.setProfile(QSurfaceFormat::CompatibilityProfile);

  format.setDepthBufferSize(24);
  format.setStencilBufferSize(8);
  format.setAlphaBufferSize(8);
  format.setBlueBufferSize(8);
  format.setRedBufferSize(8);
  format.setGreenBufferSize(8);

  format.setOption(QSurfaceFormat::DebugContext);

  // This comment is a reminder for whenever we can test using a Mac
  // machine.
  format.setMajorVersion(3);
  format.setMinorVersion(2);

  QSurfaceFormat::setDefaultFormat(format);

  QApplication a(argc, argv);
#endif

  Q_INIT_RESOURCE(resources);
  Q_INIT_RESOURCE(test_resources);

  spdlog::set_pattern("%^[%L%$][%14s:%4#] %! :: %v%$");
  spdlog::set_level(spdlog::level::trace);

  int result = Catch::Session().run(argc, argv);

  return result;
}

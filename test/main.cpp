#include "catch_wrapper.hpp"
#include "sme/logger.hpp"
#include <QApplication>
#include <catch2/catch_session.hpp>
#include <oneapi/tbb/global_control.h>

int main(int argc, char *argv[]) {
  Catch::StringMaker<double>::precision = 25;
  Catch::StringMaker<float>::precision = 25;

#ifdef SME_ENABLE_GUI_TESTS
  QApplication a(argc, argv);
#endif

  Q_INIT_RESOURCE(resources);
  Q_INIT_RESOURCE(test_resources);

  spdlog::set_pattern("%^[%L%$][%14s:%4#] %! :: %v%$");
  spdlog::set_level(spdlog::level::trace);

  int result = Catch::Session().run(argc, argv);

  return result;
}

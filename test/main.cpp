#include "catch_wrapper.hpp"
#include "logger.hpp"
#include <QApplication>
#include <catch2/catch_session.hpp>
#include <locale>

int main(int argc, char *argv[]) {
  Catch::StringMaker<double>::precision = 25;
  Catch::StringMaker<float>::precision = 25;

#ifdef SME_ENABLE_GUI_TESTS
  QApplication a(argc, argv);
#endif

  Q_INIT_RESOURCE(resources);
  Q_INIT_RESOURCE(test_resources);

  // Qt sets the locale according to the system one
  // This can cause problems with numerical code
  // e.g. when a double is "3,142" vs "3.142"
  // Here we reset the default "C" locale to avoid these issues
  std::locale::global(std::locale::classic());

  spdlog::set_pattern("%^[%L%$][%14s:%4#] %! :: %v%$");
  spdlog::set_level(spdlog::level::trace);

  int result = Catch::Session().run(argc, argv);

  return result;
}

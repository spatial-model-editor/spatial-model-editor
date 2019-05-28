#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include <QCoreApplication>
#include <QSignalSpy>

int main(int argc, char *argv[]) {
  QCoreApplication a(argc, argv);

  int result = Catch::Session().run(argc, argv);

  return result;
}
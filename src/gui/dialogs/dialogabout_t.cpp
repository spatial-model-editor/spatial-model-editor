#include "catch_wrapper.hpp"
#include "dialogabout.hpp"
#include <QLabel>

TEST_CASE("DialogAbout", "[gui/dialogs/about][gui/dialogs][gui][about]") {
  DialogAbout dia{};
  auto *lblAbout{dia.findChild<QLabel *>("lblAbout")};
  REQUIRE(lblAbout != nullptr);
  auto *lblLibraries{dia.findChild<QLabel *>("lblLibraries")};
  REQUIRE(lblLibraries != nullptr);
  REQUIRE(lblLibraries->text().contains("dune-copasi"));
  REQUIRE(lblLibraries->text().contains("libSBML"));
}

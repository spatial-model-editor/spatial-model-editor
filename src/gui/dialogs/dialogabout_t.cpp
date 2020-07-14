#include "catch_wrapper.hpp"
#include "dialogabout.hpp"
#include "version.hpp"
#include <QLabel>

TEST_CASE("DialogAbout", "[gui/dialogs/about][gui/dialogs][gui][about]") {
  auto dia = DialogAbout();
  auto *lblAbout = dia.findChild<QLabel *>("lblAbout");
  auto *lblLibraries = dia.findChild<QLabel *>("lblLibraries");
  REQUIRE(lblAbout != nullptr);
  REQUIRE(lblLibraries != nullptr);
  REQUIRE(lblLibraries->text().contains("dune-copasi"));
  REQUIRE(lblLibraries->text().contains("libSBML"));
}

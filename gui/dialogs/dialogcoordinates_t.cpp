#include "catch_wrapper.hpp"
#include "dialogcoordinates.hpp"
#include "qt_test_utils.hpp"

using namespace sme::test;

TEST_CASE("DialogCoordinates",
          "[gui/dialogs/coordinates][gui/dialogs][gui][coordinates]") {
  DialogCoordinates dia("x", "y", "z");
  REQUIRE(dia.getXName() == "x");
  REQUIRE(dia.getYName() == "y");
  REQUIRE(dia.getZName() == "z");
  ModalWidgetTimer mwt;
  mwt.addUserAction({"Delete", "Backspace", "e", "x", "!", "Tab", "Delete",
                     "Backspace", "y", "y", "Tab", "Delete", "Backspace", "h",
                     ""});
  mwt.start();
  dia.exec();
  REQUIRE(dia.getXName() == "ex!");
  REQUIRE(dia.getYName() == "yy");
  REQUIRE(dia.getZName() == "h");
}

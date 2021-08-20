#include "catch_wrapper.hpp"
#include "dialogcoordinates.hpp"
#include "qt_test_utils.hpp"

using namespace sme::test;

SCENARIO("DialogCoordinates",
         "[gui/dialogs/coordinates][gui/dialogs][gui][coordinates]") {
  DialogCoordinates dia("x", "y");
  REQUIRE(dia.getXName() == "x");
  REQUIRE(dia.getYName() == "y");
  ModalWidgetTimer mwt;
  mwt.addUserAction({"Delete", "Backspace", "e", "x", "!", "Tab", "Delete",
                     "Backspace", "y", "y"});
  mwt.start();
  dia.exec();
  REQUIRE(dia.getXName() == "ex!");
  REQUIRE(dia.getYName() == "yy");
}

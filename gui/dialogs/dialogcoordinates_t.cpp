#include "catch_wrapper.hpp"
#include "dialogcoordinates.hpp"
#include "qt_test_utils.hpp"
#include <QLineEdit>

using namespace sme::test;

struct DialogCoordinatesWidgets {
  explicit DialogCoordinatesWidgets(const DialogCoordinates *dialog) {
    GET_DIALOG_WIDGET(QLineEdit, txtXName);
    GET_DIALOG_WIDGET(QLineEdit, txtYName);
    GET_DIALOG_WIDGET(QLineEdit, txtZName);
  }
  QLineEdit *txtXName;
  QLineEdit *txtYName;
  QLineEdit *txtZName;
};

TEST_CASE("DialogCoordinates",
          "[gui/dialogs/coordinates][gui/dialogs][gui][coordinates]") {
  DialogCoordinates dia("x", "y", "z");
  dia.show();
  DialogCoordinatesWidgets widgets(&dia);
  REQUIRE(dia.getXName() == "x");
  REQUIRE(dia.getYName() == "y");
  REQUIRE(dia.getZName() == "z");
  sendKeyEvents(widgets.txtXName, {"Delete", "Backspace", "e", "x", "!"});
  sendKeyEvents(widgets.txtYName, {"Delete", "Backspace", "y", "y"});
  sendKeyEvents(widgets.txtZName, {"Delete", "Backspace", "h"});
  REQUIRE(dia.getXName() == "ex!");
  REQUIRE(dia.getYName() == "yy");
  REQUIRE(dia.getZName() == "h");
}

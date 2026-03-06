#include "catch_wrapper.hpp"
#include "dialoggeometryorigin.hpp"
#include "qt_test_utils.hpp"
#include <QLabel>
#include <QLineEdit>

using namespace sme::test;

struct DialogGeometryOriginWidgets {
  explicit DialogGeometryOriginWidgets(const DialogGeometryOrigin *dialog) {
    GET_DIALOG_WIDGET(QLineEdit, txtXOrigin);
    GET_DIALOG_WIDGET(QLineEdit, txtYOrigin);
    GET_DIALOG_WIDGET(QLineEdit, txtZOrigin);
    GET_DIALOG_WIDGET(QLabel, lblXUnits);
    GET_DIALOG_WIDGET(QLabel, lblYUnits);
    GET_DIALOG_WIDGET(QLabel, lblZUnits);
  }
  QLineEdit *txtXOrigin;
  QLineEdit *txtYOrigin;
  QLineEdit *txtZOrigin;
  QLabel *lblXUnits;
  QLabel *lblYUnits;
  QLabel *lblZUnits;
};

TEST_CASE("DialogGeometryOrigin",
          "[gui/dialogs/geometryorigin][gui/dialogs][gui][geometryorigin]") {
  DialogGeometryOrigin dia({1.25, -2.5, 0.125}, "mm");
  dia.show();
  DialogGeometryOriginWidgets widgets(&dia);
  REQUIRE(dia.getOrigin().p.x() == dbl_approx(1.25));
  REQUIRE(dia.getOrigin().p.y() == dbl_approx(-2.5));
  REQUIRE(dia.getOrigin().z == dbl_approx(0.125));
  REQUIRE(widgets.lblXUnits->text() == "mm");
  REQUIRE(widgets.lblYUnits->text() == "mm");
  REQUIRE(widgets.lblZUnits->text() == "mm");
  widgets.txtXOrigin->setText("10.5");
  widgets.txtYOrigin->setText("-8.75");
  widgets.txtZOrigin->setText("3.25");
  sendKeyEvents(&dia, {"Enter"});
  REQUIRE(dia.getOrigin().p.x() == dbl_approx(10.5));
  REQUIRE(dia.getOrigin().p.y() == dbl_approx(-8.75));
  REQUIRE(dia.getOrigin().z == dbl_approx(3.25));
}

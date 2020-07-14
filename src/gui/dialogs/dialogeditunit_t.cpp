#include "catch_wrapper.hpp"
#include "dialogeditunit.hpp"
#include "model.hpp"
#include "qt_test_utils.hpp"
#include <QFile>
#include <QLabel>
#include <QLineEdit>

TEST_CASE("DialogEditUnit",
          "[gui/dialogs/editunit][gui/dialogs][gui][editunit][units]") {
  model::Unit u{"h", "second", 0, 1, 3600};
  ModalWidgetTimer mwt;
  DialogEditUnit dia(u);
  auto *lblBaseUnits = dia.findChild<QLabel *>("lblBaseUnits");
  REQUIRE(dia.getUnit().kind == u.kind);
  REQUIRE(dia.getUnit().name == u.name);
  REQUIRE(dia.getUnit().multiplier == dbl_approx(u.multiplier));
  REQUIRE(dia.getUnit().scale == u.scale);
  REQUIRE(dia.getUnit().exponent == u.exponent);
  REQUIRE(lblBaseUnits->text() == "1 h = 3600 second");
  SECTION("valid unit") {
    mwt.addUserAction({"Delete", "Backspace", "H", " ", "!", "Tab", ".", "2",
                       "Tab", "1", "0", "Tab", "2"});
    mwt.start();
    auto result = dia.exec();
    REQUIRE(dia.getUnit().kind == u.kind);
    REQUIRE(dia.getUnit().name == "H !");
    REQUIRE(dia.getUnit().multiplier == dbl_approx(0.2));
    REQUIRE(dia.getUnit().scale == 10);
    REQUIRE(dia.getUnit().exponent == 2);
    REQUIRE(lblBaseUnits->text() == "1 H ! = (0.2 * 10^(10) second)^2");
    REQUIRE(result == QDialog::Accepted);
  }
  SECTION("invalid multiplier") {
    mwt.addUserAction({"Delete", "Backspace", "q", "Tab", ".", ",", "Tab", "1",
                       "0", "Tab", "2"});
    mwt.start();
    dia.exec();
    REQUIRE(dia.getUnit().name == "q");
    // multiplier ".," invalid: unit retains previous valid multiplier value
    REQUIRE(dia.getUnit().multiplier == dbl_approx(3600));
    REQUIRE(dia.getUnit().scale == 10);
    REQUIRE(dia.getUnit().exponent == 2);
    REQUIRE(lblBaseUnits->text() ==
            "Invalid value: Multiplier must be a double");
  }
  SECTION("invalid scale") {
    mwt.addUserAction({"Tab", "Tab", "q"});
    mwt.start();
    dia.exec();
    REQUIRE(lblBaseUnits->text() == "Invalid value: Scale must be an integer");
  }
  SECTION("invalid exponent") {
    mwt.addUserAction({"Tab", "Tab", "Tab", "q"});
    mwt.start();
    dia.exec();
    REQUIRE(lblBaseUnits->text() ==
            "Invalid value: Exponent must be an integer");
  }
}

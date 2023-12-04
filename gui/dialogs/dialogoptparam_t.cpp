#include "catch_wrapper.hpp"
#include "dialogoptparam.hpp"
#include "qt_test_utils.hpp"
#include <QComboBox>
#include <QLineEdit>

using namespace sme;
using namespace sme::test;

struct DialogOptParamWidgets {
  explicit DialogOptParamWidgets(const DialogOptParam *dialog) {
    GET_DIALOG_WIDGET(QComboBox, cmbParameter);
    GET_DIALOG_WIDGET(QLineEdit, txtLowerBound);
    GET_DIALOG_WIDGET(QLineEdit, txtUpperBound);
  }
  QComboBox *cmbParameter;
  QLineEdit *txtLowerBound;
  QLineEdit *txtUpperBound;
};

TEST_CASE("DialogOptParam", "[gui/dialogs/optparam][gui/"
                            "dialogs][gui][optimize]") {
  std::vector<simulate::OptParam> defaultOptParams;
  defaultOptParams.push_back({simulate::OptParamType::ReactionParameter,
                              "myReacOptParam1", "k1", "reac1", 0.1, 0.2});
  defaultOptParams.push_back({simulate::OptParamType::ReactionParameter,
                              "myReacOptParam2", "k7", "reac2", 0.5, 0.9});
  defaultOptParams.push_back({simulate::OptParamType::ModelParameter,
                              "myOptParam", "p1", "", 0.0, 0.05});
  auto optParam{defaultOptParams.back()};
  SECTION("no parameters") {
    DialogOptParam dia({});
    dia.show();
  }
  SECTION("no pre-selected parameter") {
    DialogOptParam dia(defaultOptParams);
    dia.show();
    REQUIRE(dia.getOptParam().optParamType ==
            simulate::OptParamType::ReactionParameter);
    REQUIRE(dia.getOptParam().name == "myReacOptParam1");
  }
  SECTION("user does nothing: correct initial values") {
    DialogOptParam dia(defaultOptParams, &optParam);
    dia.show();
    REQUIRE(dia.getOptParam().optParamType ==
            simulate::OptParamType::ModelParameter);
    REQUIRE(dia.getOptParam().name == "myOptParam");
    REQUIRE(dia.getOptParam().id == "p1");
    REQUIRE(dia.getOptParam().parentId == "");
    REQUIRE(dia.getOptParam().lowerBound == dbl_approx(0.0));
    REQUIRE(dia.getOptParam().upperBound == dbl_approx(0.05));
  }
  SECTION("user changes bounds") {
    DialogOptParam dia(defaultOptParams, &optParam);
    DialogOptParamWidgets widgets(&dia);
    dia.show();
    sendKeyEvents(widgets.txtLowerBound, {"Backspace", "Backspace", "Backspace",
                                          "0", ".", "4", "Enter"});
    sendKeyEvents(widgets.txtUpperBound,
                  {"Backspace", "Backspace", "Backspace", "1", "Enter"});
    REQUIRE(dia.getOptParam().optParamType ==
            simulate::OptParamType::ModelParameter);
    REQUIRE(dia.getOptParam().name == "myOptParam");
    REQUIRE(dia.getOptParam().id == "p1");
    REQUIRE(dia.getOptParam().parentId == "");
    REQUIRE(dia.getOptParam().lowerBound == dbl_approx(0.4));
    REQUIRE(dia.getOptParam().upperBound == dbl_approx(1));
  }
  SECTION("user changes parameter: bounds set from corresponding default") {
    DialogOptParam dia(defaultOptParams, &optParam);
    DialogOptParamWidgets widgets(&dia);
    dia.show();
    widgets.cmbParameter->setCurrentIndex(1);
    REQUIRE(dia.getOptParam().optParamType ==
            simulate::OptParamType::ReactionParameter);
    REQUIRE(dia.getOptParam().name == "myReacOptParam2");
    REQUIRE(dia.getOptParam().id == "k7");
    REQUIRE(dia.getOptParam().parentId == "reac2");
    REQUIRE(dia.getOptParam().lowerBound == dbl_approx(0.5));
    REQUIRE(dia.getOptParam().upperBound == dbl_approx(0.9));
  }
  SECTION("user changes parameter and bounds") {
    DialogOptParam dia(defaultOptParams, &optParam);
    DialogOptParamWidgets widgets(&dia);
    dia.show();
    widgets.cmbParameter->setCurrentIndex(0);
    sendKeyEvents(widgets.txtLowerBound,
                  {"Backspace", "Backspace", "Backspace", "6", "Enter"});
    sendKeyEvents(widgets.txtUpperBound,
                  {"Backspace", "Backspace", "Backspace", "1", "9", "Enter"});
    REQUIRE(dia.getOptParam().optParamType ==
            simulate::OptParamType::ReactionParameter);
    REQUIRE(dia.getOptParam().name == "myReacOptParam1");
    REQUIRE(dia.getOptParam().id == "k1");
    REQUIRE(dia.getOptParam().parentId == "reac1");
    REQUIRE(dia.getOptParam().lowerBound == dbl_approx(6));
    REQUIRE(dia.getOptParam().upperBound == dbl_approx(19));
  }
}

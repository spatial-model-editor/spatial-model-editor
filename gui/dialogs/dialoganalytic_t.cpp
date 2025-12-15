#include "catch_wrapper.hpp"
#include "dialoganalytic.hpp"
#include "model_test_utils.hpp"
#include "qplaintextmathedit.hpp"
#include "qt_test_utils.hpp"
#include "sme/model.hpp"
#include <QCheckBox>
#include <QPushButton>
#include <QSlider>

using namespace sme::test;

struct DialogAnalyticWidgets {
  explicit DialogAnalyticWidgets(const DialogAnalytic *dialog) {
    GET_DIALOG_WIDGET(QCheckBox, chkGrid);
    GET_DIALOG_WIDGET(QCheckBox, chkScale);
    GET_DIALOG_WIDGET(QPlainTextMathEdit, txtExpression);
    GET_DIALOG_WIDGET(QPushButton, btnExportImage);
    GET_DIALOG_WIDGET(QSlider, slideZIndex);
  }
  QCheckBox *chkGrid;
  QCheckBox *chkScale;
  QPlainTextMathEdit *txtExpression;
  QPushButton *btnExportImage;
  QSlider *slideZIndex;
};

TEST_CASE("DialogAnalytic",
          "[gui/dialogs/analytic][gui/dialogs][gui][analytic]") {
  SECTION("10x10 image, small compartment, simple expr") {
    auto model{getExampleModel(Mod::ABtoC)};
    auto compartmentVoxels = std::vector<sme::common::Voxel>{
        {5, 5, 0}, {5, 6, 0}, {5, 7, 0}, {6, 6, 0}, {6, 7, 0}};
    DialogAnalytic dia("x", DialogAnalyticDataType::Concentration,
                       {{10, 10, 1},
                        compartmentVoxels,
                        {0.0, 0.0, 0.0},
                        {1.0, 1.0, 1.0},
                        model.getUnits()},
                       model.getParameters(), model.getFunctions(), false);
    dia.show();
    DialogAnalyticWidgets widgets(&dia);
    REQUIRE(dia.getExpression() == "x");
    SECTION("valid expr: 10") {
      sendKeyEvents(widgets.txtExpression, {"Delete", "1", "0"});
      REQUIRE(dia.isExpressionValid() == true);
      REQUIRE(dia.getExpression() == "10");
    }
    SECTION("invalid syntax: (") {
      sendKeyEvents(widgets.txtExpression, {"Delete", "(", "Left"});
      REQUIRE(dia.isExpressionValid() == false);
      REQUIRE(dia.getExpression().empty() == true);
    }
    SECTION("illegal syntax: &") {
      sendKeyEvents(widgets.txtExpression, {"Delete", "&"});
      REQUIRE(dia.isExpressionValid() == false);
      REQUIRE(dia.getExpression().empty() == true);
    }
    SECTION("unknown variable: q") {
      sendKeyEvents(widgets.txtExpression, {"Delete", "q"});
      REQUIRE(dia.isExpressionValid() == false);
      REQUIRE(dia.getExpression().empty() == true);
    }
    SECTION("1/0") {
      // https://github.com/spatial-model-editor/spatial-model-editor/issues/805
      sendKeyEvents(widgets.txtExpression, {"Delete", "1", "/", "0"});
      REQUIRE(dia.isExpressionValid() == false);
      REQUIRE(dia.getExpression().empty() == true);
    }
    SECTION("nan") {
      sendKeyEvents(widgets.txtExpression, {"Delete", "n", "a", "n"});
      REQUIRE(dia.isExpressionValid() == false);
      REQUIRE(dia.getExpression().empty() == true);
    }
    SECTION("inf") {
      sendKeyEvents(widgets.txtExpression, {"Delete", "i", "n", "f"});
      REQUIRE(dia.isExpressionValid() == false);
      REQUIRE(dia.getExpression().empty() == true);
    }
    SECTION("unknown function: sillyfunc") {
      sendKeyEvents(widgets.txtExpression, {"Delete", "s", "i", "l", "l", "y",
                                            "f", "u", "n", "c", "(", "x", ")"});
      REQUIRE(dia.isExpressionValid() == false);
      REQUIRE(dia.getExpression().empty() == true);
    }
    SECTION("negative expr: sin(x)") {
      sendKeyEvents(widgets.txtExpression,
                    {"Delete", "s", "i", "n", "(", "x", ")", "Left", "Left",
                     "Left", "Left"});
      REQUIRE(dia.isExpressionValid() == false);
      REQUIRE(dia.getExpression().empty() == true);
    }
    SECTION("valid expr: 1.5 + sin(x)") {
      sendKeyEvents(widgets.txtExpression, {"Delete", "1", ".", "5", " ", "+",
                                            " ", "s", "i", "n", "(", "x", ")"});
      REQUIRE(dia.isExpressionValid() == true);
      REQUIRE(dia.getExpression() == "1.5 + sin(x)");
    }
    SECTION("valid expr: 1.5 + sin(x) & export image") {
      ModalWidgetTimer mwt;
      mwt.setIgnoredWidget(&dia);
      mwt.addUserAction(
          {"a", "n", "a", "l", "y", "t", "i", "c", "C", "o", "n", "c"});
      mwt.start();
      sendKeyEvents(widgets.txtExpression,
                    {"Delete", "1", ".", "5", " ", "+", " ", "s", "i", "n", "(",
                     "x", ")", "Tab", "Space"});
      sendMouseClick(widgets.btnExportImage);
      REQUIRE(dia.isExpressionValid() == true);
      REQUIRE(dia.getExpression() == "1.5 + sin(x)");
      QImage img("analyticConc.png");
      REQUIRE(img.size() == QSize(10, 10));
    }
  }
  SECTION("100x100 image") {
    auto model{getExampleModel(Mod::ABtoC)};
    DialogAnalytic dia("x", DialogAnalyticDataType::Concentration,
                       model.getSpeciesGeometry("B"), model.getParameters(),
                       model.getFunctions(), false);
    dia.show();
    DialogAnalyticWidgets widgets(&dia);
    REQUIRE(dia.getExpression() == "x");
    SECTION("valid expr: 10 & unclick grid/scale checkboxes") {
      sendKeyEvents(widgets.txtExpression, {"Delete", "1", "0"});
      sendMouseClick(widgets.chkGrid);
      sendMouseClick(widgets.chkScale);
      REQUIRE(dia.isExpressionValid() == true);
      REQUIRE(dia.getExpression() == "10");
    }
  }
}

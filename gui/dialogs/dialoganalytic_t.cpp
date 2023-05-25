#include "catch_wrapper.hpp"
#include "dialoganalytic.hpp"
#include "model_test_utils.hpp"
#include "qt_test_utils.hpp"
#include "sme/model.hpp"

using namespace sme::test;

TEST_CASE("DialogAnalytic",
          "[gui/dialogs/analytic][gui/dialogs][gui][analytic]") {
  SECTION("10x10 image, small compartment, simple expr") {
    auto model{getExampleModel(Mod::ABtoC)};
    auto compartmentVoxels = std::vector<sme::common::Voxel>{
        {5, 5, 0}, {5, 6, 0}, {5, 7, 0}, {6, 6, 0}, {6, 7, 0}};
    DialogAnalytic dia("x",
                       {{10, 10, 1},
                        compartmentVoxels,
                        {0.0, 0.0, 0.0},
                        {1.0, 1.0, 1.0},
                        model.getUnits()},
                       model.getParameters(), model.getFunctions(), false);
    REQUIRE(dia.getExpression() == "x");
    ModalWidgetTimer mwt;
    SECTION("valid expr: 10") {
      mwt.addUserAction({"Delete", "1", "0"});
      mwt.start();
      dia.exec();
      REQUIRE(dia.isExpressionValid() == true);
      REQUIRE(dia.getExpression() == "10");
    }
    SECTION("invalid syntax: (") {
      mwt.addUserAction({"Delete", "(", "Left"});
      mwt.start();
      dia.exec();
      REQUIRE(dia.isExpressionValid() == false);
      REQUIRE(dia.getExpression().empty() == true);
    }
    SECTION("illegal syntax: &") {
      mwt.addUserAction({"Delete", "&"});
      mwt.start();
      dia.exec();
      REQUIRE(dia.isExpressionValid() == false);
      REQUIRE(dia.getExpression().empty() == true);
    }
    SECTION("unknown variable: q") {
      mwt.addUserAction({"Delete", "q"});
      mwt.start();
      dia.exec();
      REQUIRE(dia.isExpressionValid() == false);
      REQUIRE(dia.getExpression().empty() == true);
    }
    SECTION("1/0") {
      // https://github.com/spatial-model-editor/spatial-model-editor/issues/805
      mwt.addUserAction({"Delete", "1", "/", "0"});
      mwt.start();
      dia.exec();
      REQUIRE(dia.isExpressionValid() == false);
      REQUIRE(dia.getExpression().empty() == true);
    }
    SECTION("nan") {
      mwt.addUserAction({"Delete", "n", "a", "n"});
      mwt.start();
      dia.exec();
      REQUIRE(dia.isExpressionValid() == false);
      REQUIRE(dia.getExpression().empty() == true);
    }
    SECTION("inf") {
      mwt.addUserAction({"Delete", "i", "n", "f"});
      mwt.start();
      dia.exec();
      REQUIRE(dia.isExpressionValid() == false);
      REQUIRE(dia.getExpression().empty() == true);
    }
    SECTION("unknown function: sillyfunc") {
      mwt.addUserAction({"Delete", "s", "i", "l", "l", "y", "f", "u", "n", "c",
                         "(", "x", ")"});
      mwt.start();
      dia.exec();
      REQUIRE(dia.isExpressionValid() == false);
      REQUIRE(dia.getExpression().empty() == true);
    }
    SECTION("negative expr: sin(x)") {
      mwt.addUserAction({"Delete", "s", "i", "n", "(", "x", ")", "Left", "Left",
                         "Left", "Left"});
      mwt.start();
      dia.exec();
      REQUIRE(dia.isExpressionValid() == false);
      REQUIRE(dia.getExpression().empty() == true);
    }
    SECTION("valid expr: 1.5 + sin(x)") {
      mwt.addUserAction({"Delete", "1", ".", "5", " ", "+", " ", "s", "i", "n",
                         "(", "x", ")"});
      mwt.start();
      dia.exec();
      REQUIRE(dia.isExpressionValid() == true);
      REQUIRE(dia.getExpression() == "1.5 + sin(x)");
    }
    SECTION("valid expr: 1.5 + sin(x) & export image") {
      ModalWidgetTimer mwt2;
      mwt2.addUserAction(
          {"a", "n", "a", "l", "y", "t", "i", "c", "C", "o", "n", "c"});
      mwt.addUserAction({"Delete", "1", ".", "5", " ", "+", " ", "s", "i", "n",
                         "(", "x", ")", "Tab", "Space"},
                        true, &mwt2);
      mwt.start();
      dia.exec();
      REQUIRE(dia.isExpressionValid() == true);
      REQUIRE(dia.getExpression() == "1.5 + sin(x)");
      QImage img("analyticConc.png");
      REQUIRE(img.size() == QSize(10, 10));
    }
  }
  SECTION("100x100 image") {
    auto model{getExampleModel(Mod::ABtoC)};
    DialogAnalytic dia("x", model.getSpeciesGeometry("B"),
                       model.getParameters(), model.getFunctions(), false);
    REQUIRE(dia.getExpression() == "x");
    ModalWidgetTimer mwt;
    SECTION("valid expr: 10 & unclick grid/scale checkboxes") {
      mwt.addUserAction(
          {"Delete", "1", "0", "Shift+Tab", "Space", "Shift+Tab", "Space"});
      mwt.start();
      dia.exec();
      REQUIRE(dia.isExpressionValid() == true);
      REQUIRE(dia.getExpression() == "10");
    }
  }
}

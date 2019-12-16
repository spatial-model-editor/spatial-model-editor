#include <QFile>

#include "catch_wrapper.hpp"
#include "dialoganalytic.hpp"
#include "qt_test_utils.hpp"
#include "sbml.hpp"

SCENARIO("DialogAnalytic", "[gui][dialogs][concentration][analytic]") {
  GIVEN("10x10 image, small compartment, simple expr") {
    sbml::SbmlDocWrapper doc;
    QFile f(":/models/ABtoC.xml");
    f.open(QIODevice::ReadOnly);
    doc.importSBMLString(f.readAll().toStdString());
    auto compartmentPoints = std::vector<QPoint>{
        QPoint(5, 5), QPoint(5, 6), QPoint(5, 7), QPoint(6, 6), QPoint(6, 7)};
    DialogAnalytic dia("x", {QSize(10, 10), compartmentPoints,
                             QPointF(0.0, 0.0), 1, doc.getModelUnits()});
    REQUIRE(dia.getExpression() == "x");
    ModalWidgetTimer mwt;
    WHEN("valid expr: 10") {
      mwt.addUserAction({"Delete", "1", "0"});
      mwt.start();
      dia.exec();
      REQUIRE(dia.isExpressionValid() == true);
      REQUIRE(dia.getExpression() == "10");
    }
    WHEN("invalid expr: (") {
      mwt.addUserAction({"Delete", "(", "Left"});
      mwt.start();
      dia.exec();
      REQUIRE(dia.isExpressionValid() == false);
      REQUIRE(dia.getExpression().empty() == true);
    }
    WHEN("illegal char: &") {
      mwt.addUserAction({"Delete", "&"});
      mwt.start();
      dia.exec();
      REQUIRE(dia.isExpressionValid() == false);
      REQUIRE(dia.getExpression().empty() == true);
    }
    WHEN("invalid expr: q") {
      mwt.addUserAction({"Delete", "q"});
      mwt.start();
      dia.exec();
      REQUIRE(dia.isExpressionValid() == false);
      REQUIRE(dia.getExpression().empty() == true);
    }
    WHEN("negative expr: sin(x)") {
      mwt.addUserAction({"Delete", "s", "i", "n", "(", "x", ")", "Left", "Left",
                         "Left", "Left"});
      mwt.start();
      dia.exec();
      REQUIRE(dia.isExpressionValid() == false);
      REQUIRE(dia.getExpression().empty() == true);
    }
    WHEN("valid expr: 1.5 + sin(x)") {
      mwt.addUserAction({"Delete", "1", ".", "5", " ", "+", " ", "s", "i", "n",
                         "(", "x", ")"});
      mwt.start();
      dia.exec();
      REQUIRE(dia.isExpressionValid() == true);
      REQUIRE(dia.getExpression() == "1.5 + sin(x)");
    }
  }
  GIVEN("100x100 image") {
    sbml::SbmlDocWrapper doc;
    QFile f(":/models/ABtoC.xml");
    f.open(QIODevice::ReadOnly);
    doc.importSBMLString(f.readAll().toStdString());
    DialogAnalytic dia("x", doc.getSpeciesGeometry("B"));
    REQUIRE(dia.getExpression() == "x");
    ModalWidgetTimer mwt;
    WHEN("valid expr: 10") {
      mwt.addUserAction({"Delete", "1", "0"});
      mwt.start();
      dia.exec();
      REQUIRE(dia.isExpressionValid() == true);
      REQUIRE(dia.getExpression() == "10");
    }
  }
}

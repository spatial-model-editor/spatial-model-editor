#include <QFile>

#include "catch_wrapper.hpp"
#include "dialogimagesize.hpp"
#include "qt_test_utils.hpp"
#include "sbml.hpp"

SCENARIO("DialogImageSize", "[gui][dialogs][imagesize]") {
  GIVEN("100x50 image, initial pixel size 1") {
    sbml::SbmlDocWrapper doc;
    QFile f(":/models/ABtoC.xml");
    f.open(QIODevice::ReadOnly);
    doc.importSBMLString(f.readAll().toStdString());
    auto modelUnits = doc.getModelUnits();
    modelUnits.setLength(0);
    REQUIRE(modelUnits.getLengthUnits()[0].scale == 0);
    REQUIRE(modelUnits.getLengthUnits()[1].scale == -1);
    REQUIRE(modelUnits.getLengthUnits()[2].scale == -2);
    REQUIRE(modelUnits.getLengthUnits()[3].scale == -3);
    REQUIRE(modelUnits.getLengthUnits()[4].scale == -6);
    QImage img(100, 50, QImage::Format_ARGB32_Premultiplied);
    img.fill(0xFFFFFFFF);
    DialogImageSize dim(img, 1.0, modelUnits);
    REQUIRE(dim.getPixelWidth() == dbl_approx(1.0));
    ModalWidgetTimer mwt;
    WHEN("user sets width to 1, same units, pixel size -> 0.01") {
      mwt.addUserAction({"1"});
      mwt.start();
      dim.exec();
      REQUIRE(dim.getPixelWidth() == dbl_approx(0.01));
    }
    WHEN("user sets width to 1e-8, pixel size -> 1e-10") {
      mwt.addUserAction({"1", "e", "-", "8"});
      mwt.start();
      dim.exec();
      REQUIRE(dim.getPixelWidth() == dbl_approx(1e-10));
    }
    WHEN("user sets height to 10, pixel size -> 0.2") {
      mwt.addUserAction({"Tab", "Tab", "1", "0"});
      mwt.start();
      dim.exec();
      REQUIRE(dim.getPixelWidth() == dbl_approx(0.2));
    }
    WHEN("user sets height to 10, units to dm, pixel size -> 0.02") {
      mwt.addUserAction({"Tab", "Down", "Tab", "1", "0"});
      mwt.start();
      dim.exec();
      REQUIRE(dim.getPixelWidth() == dbl_approx(0.02));
    }
    WHEN("user sets height to 10, units to cm, pixel size -> 0.002") {
      mwt.addUserAction({"Tab", "Down", "Down", "Tab", "1", "0"});
      mwt.start();
      dim.exec();
      REQUIRE(dim.getPixelWidth() == dbl_approx(0.002));
    }
    WHEN(
        "user sets width to 9 & units to dm, then height 10, units um,"
        " pixel size -> 0.0000002") {
      mwt.addUserAction(
          {"9", "Tab", "Down", "Tab", "1", "0", "Tab", "Down", "Down", "Down"});
      mwt.start();
      dim.exec();
      REQUIRE(dim.getPixelWidth() == dbl_approx(0.0000002));
    }
    WHEN("default units changed to mm, user sets width = 1, units = m") {
      modelUnits.setLength(3);  // mm
      DialogImageSize dim2(img, 22.0, modelUnits);
      REQUIRE(dim2.getPixelWidth() == dbl_approx(22.0));
      mwt.addUserAction({"1", "Tab", "Up", "Up", "Up", "Up", "Up", "Up"});
      mwt.start();
      dim2.exec();
      REQUIRE(dim2.getPixelWidth() == dbl_approx(10.0));
    }
  }
}

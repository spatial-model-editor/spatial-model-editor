#include "catch_wrapper.hpp"
#include "dialoggeometryimage.hpp"
#include "model.hpp"
#include "qlabelmousetracker.hpp"
#include <QPushButton>
#include "qt_test_utils.hpp"
#include <QFile>

SCENARIO("DialogGeometryImage",
         "[gui/dialogs/geometryimage][gui/dialogs][gui][geometryimage]") {
  GIVEN("100x50 image, initial pixel size 1") {
    sme::model::Model doc;
    QFile f(":/models/ABtoC.xml");
    f.open(QIODevice::ReadOnly);
    doc.importSBMLString(f.readAll().toStdString());
    auto modelUnits = doc.getUnits();
    modelUnits.setLengthIndex(0);
    REQUIRE(modelUnits.getLengthUnits()[0].scale == 0);
    REQUIRE(modelUnits.getLengthUnits()[1].scale == -1);
    REQUIRE(modelUnits.getLengthUnits()[2].scale == -2);
    REQUIRE(modelUnits.getLengthUnits()[3].scale == -3);
    REQUIRE(modelUnits.getLengthUnits()[4].scale == -6);
    QImage img(100, 50, QImage::Format_ARGB32_Premultiplied);
    img.fill(qRgb(255, 255, 255));
    img.setPixel(50, 25, qRgb(123, 123, 123));
    img.setPixel(55, 23, qRgba(123, 123, 101, 123));
    DialogGeometryImage dim(img, 1.0, modelUnits);
    REQUIRE(dim.getPixelWidth() == dbl_approx(1.0));
    ModalWidgetTimer mwt;
    auto* lblImage{dim.findChild<QLabelMouseTracker*>("lblImage")};
    REQUIRE(lblImage != nullptr);
    auto* btnSelectColours{dim.findChild<QPushButton*>("btnSelectColours")};
    REQUIRE(btnSelectColours != nullptr);
    auto* btnApplyColours{dim.findChild<QPushButton*>("btnApplyColours")};
    REQUIRE(btnApplyColours != nullptr);
    auto* btnResetColours{dim.findChild<QPushButton*>("btnResetColours")};
    REQUIRE(btnResetColours != nullptr);
    WHEN("user sets width to 1, same units, pixel size -> 0.01") {
      mwt.addUserAction({"1"});
      mwt.start();
      dim.exec();
      REQUIRE(dim.getPixelWidth() == dbl_approx(0.01));
      REQUIRE(dim.imageAltered() == false);
    }
    WHEN("user sets width to 1e-8, pixel size -> 1e-10") {
      mwt.addUserAction({"1", "e", "-", "8"});
      mwt.start();
      dim.exec();
      REQUIRE(dim.getPixelWidth() == dbl_approx(1e-10));
      REQUIRE(dim.imageAltered() == false);
    }
    WHEN("user sets height to 10, pixel size -> 0.2") {
      mwt.addUserAction({"Tab", "Tab", "1", "0"});
      mwt.start();
      dim.exec();
      REQUIRE(dim.getPixelWidth() == dbl_approx(0.2));
      REQUIRE(dim.imageAltered() == false);
    }
    WHEN("user sets height to 10, units to dm, pixel size -> 0.02") {
      mwt.addUserAction({"Tab", "Down", "Tab", "1", "0"});
      mwt.start();
      dim.exec();
      REQUIRE(dim.getPixelWidth() == dbl_approx(0.02));
      REQUIRE(dim.imageAltered() == false);
    }
    WHEN("user sets height to 10, units to cm, pixel size -> 0.002") {
      mwt.addUserAction({"Tab", "Down", "Down", "Tab", "1", "0"});
      mwt.start();
      dim.exec();
      REQUIRE(dim.getPixelWidth() == dbl_approx(0.002));
      REQUIRE(dim.imageAltered() == false);
    }
    WHEN("user sets width to 9 & units to dm, then height 10, units um,"
         " pixel size -> 0.0000002") {
      mwt.addUserAction(
          {"9", "Tab", "Down", "Tab", "1", "0", "Tab", "Down", "Down", "Down"});
      mwt.start();
      dim.exec();
      REQUIRE(dim.getPixelWidth() == dbl_approx(0.0000002));
      REQUIRE(dim.imageAltered() == false);
    }
    WHEN("default units changed to mm, user sets width = 1, units = m") {
      modelUnits.setLengthIndex(3); // mm
      DialogGeometryImage dim2(img, 22.0, modelUnits);
      REQUIRE(dim2.getPixelWidth() == dbl_approx(22.0));
      mwt.addUserAction({"1", "Tab", "Up", "Up", "Up", "Up", "Up", "Up"});
      mwt.start();
      dim2.exec();
      REQUIRE(dim2.getPixelWidth() == dbl_approx(10.0));
      REQUIRE(dim2.imageAltered() == false);
    }
    WHEN("x pixels reduced from 100 to 98") {
      REQUIRE(dim.getPixelWidth() == dbl_approx(1.0));
      REQUIRE(dim.imageAltered() == false);
      REQUIRE(dim.getAlteredImage().width() == 100);
      REQUIRE(dim.getAlteredImage().height() == 50);
      mwt.addUserAction({"Tab", "Tab", "Tab", "Tab", "Down", "Down"});
      mwt.start();
      dim.exec();
      REQUIRE(dim.imageAltered() == true);
      REQUIRE(dim.getPixelWidth() == dbl_approx(1.0 / 0.98));
      REQUIRE(dim.getAlteredImage().width() == 98);
      REQUIRE(dim.getAlteredImage().height() == 49);
    }
    WHEN("y pixels reduced from 50 to 5") {
      REQUIRE(dim.getPixelWidth() == dbl_approx(1.0));
      REQUIRE(dim.imageAltered() == false);
      REQUIRE(dim.getAlteredImage().width() == 100);
      REQUIRE(dim.getAlteredImage().height() == 50);
      mwt.addUserAction({"Tab", "Tab", "Tab", "Tab", "Tab", "5"});
      mwt.start();
      dim.exec();
      REQUIRE(dim.imageAltered() == true);
      REQUIRE(dim.getPixelWidth() == dbl_approx(10.0));
      REQUIRE(dim.getAlteredImage().width() == 10);
      REQUIRE(dim.getAlteredImage().height() == 5);
    }
    WHEN("x pixels reduced, y pixels reduced, reset pixels pressed") {
      REQUIRE(dim.getPixelWidth() == dbl_approx(1.0));
      REQUIRE(dim.imageAltered() == false);
      REQUIRE(dim.getAlteredImage().width() == 100);
      REQUIRE(dim.getAlteredImage().height() == 50);
      mwt.addUserAction({"Tab", "Tab", "Tab", "Tab", "Down", "Tab", "5", "Tab",
                         "Space", "Tab", "Tab", "Tab"});
      mwt.start();
      dim.exec();
      REQUIRE(dim.imageAltered() == false);
      REQUIRE(dim.getAlteredImage().width() == 100);
      REQUIRE(dim.getAlteredImage().height() == 50);
      // width/height both altered as aspect ratio changes slightly due to
      // integer pixel count, so pixel width is not exactly preserved
      REQUIRE(std::abs(dim.getPixelWidth() - 1.0) < 0.02);
    }
    WHEN("select colours pressed, then reset colours") {
      REQUIRE(dim.getPixelWidth() == dbl_approx(1.0));
      REQUIRE(dim.imageAltered() == false);
      REQUIRE(dim.getAlteredImage().width() == 100);
      REQUIRE(dim.getAlteredImage().height() == 50);
      mwt.addUserAction({"Tab", "Tab", "Tab", "Tab", "Tab", "Tab", "Tab",
                         "Space", "Space", "Tab"});
      mwt.start();
      dim.exec();
      REQUIRE(dim.getPixelWidth() == dbl_approx(1.0));
      REQUIRE(dim.imageAltered() == false);
      REQUIRE(dim.getAlteredImage().width() == 100);
      REQUIRE(dim.getAlteredImage().height() == 50);
    }
    WHEN("select colours pressed, then one chosen") {
      dim.show();
      REQUIRE(dim.getPixelWidth() == dbl_approx(1.0));
      REQUIRE(dim.imageAltered() == false);
      REQUIRE(dim.getAlteredImage().width() == 100);
      REQUIRE(dim.getAlteredImage().height() == 50);
      REQUIRE(dim.getAlteredImage().colorCount() == 3);
      sendMouseClick(btnSelectColours);
      sendMouseClick(lblImage, QPoint(3, 3));
      sendMouseClick(btnApplyColours);
      REQUIRE(dim.getPixelWidth() == dbl_approx(1.0));
      REQUIRE(dim.imageAltered() == true);
      REQUIRE(dim.getAlteredImage().width() == 100);
      REQUIRE(dim.getAlteredImage().height() == 50);
      REQUIRE(dim.getAlteredImage().colorCount() == 1);
    }
  }
}

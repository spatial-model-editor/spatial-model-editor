#include "catch_wrapper.hpp"
#include "dialoggeometryimage.hpp"
#include "model_test_utils.hpp"
#include "qlabelmousetracker.hpp"
#include "qt_test_utils.hpp"
#include "sme/model.hpp"
#include <QDialogButtonBox>
#include <QPushButton>

using namespace sme::test;

TEST_CASE("DialogGeometryImage",
          "[gui/dialogs/geometryimage][gui/dialogs][gui][geometryimage]") {
  auto m{getExampleModel(Mod::ABtoC)};
  auto modelUnits = m.getUnits();
  modelUnits.setLengthIndex(0);
  REQUIRE(modelUnits.getLengthUnits()[0].scale == 0);
  REQUIRE(modelUnits.getLengthUnits()[1].scale == -1);
  REQUIRE(modelUnits.getLengthUnits()[2].scale == -2);
  REQUIRE(modelUnits.getLengthUnits()[3].scale == -3);
  REQUIRE(modelUnits.getLengthUnits()[4].scale == -6);
  sme::common::ImageStack img({100, 50, 1},
                              QImage::Format_ARGB32_Premultiplied);
  img.fill(qRgb(255, 255, 255));
  img[0].setPixel(50, 25, qRgb(123, 123, 123));
  img[0].setPixel(55, 23, qRgba(123, 123, 101, 123));
  DialogGeometryImage dim(img, {1.0, 1.0, 1.0}, modelUnits);
  REQUIRE(dim.getVoxelSize().width() == dbl_approx(1.0));
  REQUIRE(dim.getVoxelSize().height() == dbl_approx(1.0));
  REQUIRE(dim.getVoxelSize().depth() == dbl_approx(1.0));
  ModalWidgetTimer mwt;
  auto *lblImage{dim.findChild<QLabelMouseTracker *>("lblImage")};
  REQUIRE(lblImage != nullptr);
  auto *okButton{dim.findChild<QDialogButtonBox *>("buttonBox")
                     ->button(QDialogButtonBox::Ok)};
  REQUIRE(okButton != nullptr);
  auto *btnSelectColours{dim.findChild<QPushButton *>("btnSelectColours")};
  REQUIRE(btnSelectColours != nullptr);
  auto *btnApplyColours{dim.findChild<QPushButton *>("btnApplyColours")};
  REQUIRE(btnApplyColours != nullptr);
  auto *btnResetColours{dim.findChild<QPushButton *>("btnResetColours")};
  REQUIRE(btnResetColours != nullptr);
  SECTION("user sets width to 1, same units, width -> 0.01") {
    mwt.addUserAction({"Tab", "1"});
    mwt.start();
    dim.exec();
    REQUIRE(dim.getVoxelSize().width() == dbl_approx(0.01));
    REQUIRE(dim.getVoxelSize().height() == dbl_approx(1.0));
    REQUIRE(dim.getVoxelSize().depth() == dbl_approx(1.0));
    REQUIRE(dim.imageSizeAltered() == false);
    REQUIRE(dim.imageColoursAltered() == false);
  }
  SECTION("user sets width to 1e-8, width -> 1e-10") {
    mwt.addUserAction({"Tab", "1", "e", "-", "8"});
    mwt.start();
    dim.exec();
    REQUIRE(dim.getVoxelSize().width() == dbl_approx(1e-10));
    REQUIRE(dim.getVoxelSize().height() == dbl_approx(1.0));
    REQUIRE(dim.getVoxelSize().depth() == dbl_approx(1.0));
    REQUIRE(dim.imageSizeAltered() == false);
    REQUIRE(dim.imageColoursAltered() == false);
  }
  SECTION("user sets height to 10, height -> 0.2") {
    mwt.addUserAction({"Tab", "Tab", "Tab", "1", "0"});
    mwt.start();
    dim.exec();
    REQUIRE(dim.getVoxelSize().width() == dbl_approx(1.0));
    REQUIRE(dim.getVoxelSize().height() == dbl_approx(0.2));
    REQUIRE(dim.getVoxelSize().depth() == dbl_approx(1.0));
    REQUIRE(dim.imageSizeAltered() == false);
    REQUIRE(dim.imageColoursAltered() == false);
  }
  SECTION("user sets height to 10, units to dm, height -> 0.02, others /=10") {
    mwt.addUserAction({"Tab", "Tab", "Down", "Tab", "1", "0"});
    mwt.start();
    dim.exec();
    REQUIRE(dim.getVoxelSize().width() == dbl_approx(0.1));
    REQUIRE(dim.getVoxelSize().height() == dbl_approx(0.02));
    REQUIRE(dim.getVoxelSize().depth() == dbl_approx(0.1));
    REQUIRE(dim.imageSizeAltered() == false);
    REQUIRE(dim.imageColoursAltered() == false);
  }
  SECTION(
      "user sets height to 10, units to cm, height -> 0.002, others /=100") {
    mwt.addUserAction({"Tab", "Tab", "Down", "Down", "Tab", "1", "0"});
    mwt.start();
    dim.exec();
    REQUIRE(dim.getVoxelSize().width() == dbl_approx(0.01));
    REQUIRE(dim.getVoxelSize().height() == dbl_approx(0.002));
    REQUIRE(dim.getVoxelSize().depth() == dbl_approx(0.01));
    REQUIRE(dim.imageSizeAltered() == false);
    REQUIRE(dim.imageColoursAltered() == false);
  }
  SECTION("user sets width to 9 & units to dm, then height 10, units um") {
    mwt.addUserAction({"Tab", "9", "Tab", "Down", "Tab", "1", "0", "Shift+Tab",
                       "Down", "Down", "Down"});
    mwt.start();
    dim.exec();
    REQUIRE(dim.getVoxelSize().width() == dbl_approx(0.09 * 1e-6));
    REQUIRE(dim.getVoxelSize().height() == dbl_approx(0.2 * 1e-6));
    REQUIRE(dim.getVoxelSize().depth() == dbl_approx(1e-6));
    REQUIRE(dim.imageSizeAltered() == false);
    REQUIRE(dim.imageColoursAltered() == false);
  }
  SECTION("user sets depth to 4, no change of units") {
    mwt.addUserAction({"Tab", "Tab", "Tab", "Tab", "Backspace", "4"});
    mwt.start();
    dim.exec();
    REQUIRE(dim.getVoxelSize().width() == dbl_approx(1.0));
    REQUIRE(dim.getVoxelSize().height() == dbl_approx(1.0));
    REQUIRE(dim.getVoxelSize().depth() == dbl_approx(4.0));
    REQUIRE(dim.imageSizeAltered() == false);
    REQUIRE(dim.imageColoursAltered() == false);
  }
  SECTION("default units changed to mm, user sets width = 1, units = m") {
    modelUnits.setLengthIndex(3); // mm
    DialogGeometryImage dim2(img, {22.0, 22.0, 1.0}, modelUnits);
    REQUIRE(dim2.getVoxelSize().width() == dbl_approx(22.0));
    REQUIRE(dim2.getVoxelSize().height() == dbl_approx(22.0));
    REQUIRE(dim2.getVoxelSize().depth() == dbl_approx(1.0));
    mwt.addUserAction({"Tab", "1", "Tab", "Up", "Up", "Up", "Up", "Up", "Up"});
    mwt.start();
    dim2.exec();
    REQUIRE(dim2.getVoxelSize().width() == dbl_approx(10.0));
    REQUIRE(dim2.getVoxelSize().height() == dbl_approx(22e3));
    REQUIRE(dim2.getVoxelSize().depth() == dbl_approx(1e3));
    REQUIRE(dim.imageSizeAltered() == false);
    REQUIRE(dim.imageColoursAltered() == false);
  }
  SECTION("x pixels reduced from 100 to 98") {
    REQUIRE(dim.getVoxelSize().width() == dbl_approx(1.0));
    REQUIRE(dim.getVoxelSize().height() == dbl_approx(1.0));
    REQUIRE(dim.getVoxelSize().depth() == dbl_approx(1.0));
    REQUIRE(dim.imageSizeAltered() == false);
    REQUIRE(dim.imageColoursAltered() == false);
    REQUIRE(dim.getAlteredImage().volume().width() == 100);
    REQUIRE(dim.getAlteredImage().volume().height() == 50);
    REQUIRE(dim.getAlteredImage().volume().depth() == 1);
    mwt.addUserAction(
        {"Tab", "Tab", "Tab", "Tab", "Tab", "Tab", "Down", "Down"});
    mwt.start();
    dim.exec();
    REQUIRE(dim.imageSizeAltered() == true);
    REQUIRE(dim.imageColoursAltered() == false);
    REQUIRE(dim.getVoxelSize().width() == dbl_approx(1.0 / 0.98));
    REQUIRE(dim.getVoxelSize().height() == dbl_approx(1.0 / 0.98));
    REQUIRE(dim.getVoxelSize().depth() == dbl_approx(1.0));
    REQUIRE(dim.getAlteredImage().volume().width() == 98);
    REQUIRE(dim.getAlteredImage().volume().height() == 49);
    REQUIRE(dim.getAlteredImage().volume().depth() == 1);
  }
  SECTION("y pixels reduced from 50 to 5") {
    REQUIRE(dim.getVoxelSize().width() == dbl_approx(1.0));
    REQUIRE(dim.getVoxelSize().height() == dbl_approx(1.0));
    REQUIRE(dim.getVoxelSize().depth() == dbl_approx(1.0));
    REQUIRE(dim.imageSizeAltered() == false);
    REQUIRE(dim.imageColoursAltered() == false);
    REQUIRE(dim.getAlteredImage().volume().width() == 100);
    REQUIRE(dim.getAlteredImage().volume().height() == 50);
    REQUIRE(dim.getAlteredImage().volume().depth() == 1);
    mwt.addUserAction({"Tab", "Tab", "Tab", "Tab", "Tab", "Tab", "Tab", "5"});
    mwt.start();
    dim.exec();
    REQUIRE(dim.imageSizeAltered() == true);
    REQUIRE(dim.imageColoursAltered() == false);
    REQUIRE(dim.getVoxelSize().width() == dbl_approx(10.0));
    REQUIRE(dim.getVoxelSize().height() == dbl_approx(10.0));
    REQUIRE(dim.getVoxelSize().depth() == dbl_approx(1.0));
    REQUIRE(dim.getAlteredImage().volume().width() == 10);
    REQUIRE(dim.getAlteredImage().volume().height() == 5);
    REQUIRE(dim.getAlteredImage().volume().depth() == 1);
  }
  SECTION("x pixels reduced, y pixels reduced, reset pixels pressed") {
    REQUIRE(dim.getVoxelSize().width() == dbl_approx(1.0));
    REQUIRE(dim.getVoxelSize().height() == dbl_approx(1.0));
    REQUIRE(dim.getVoxelSize().depth() == dbl_approx(1.0));
    REQUIRE(dim.imageSizeAltered() == false);
    REQUIRE(dim.imageColoursAltered() == false);
    REQUIRE(dim.getAlteredImage().volume().width() == 100);
    REQUIRE(dim.getAlteredImage().volume().height() == 50);
    REQUIRE(dim.getAlteredImage().volume().depth() == 1);
    mwt.addUserAction({"Tab", "Tab", "Tab", "Tab", "Tab", "Down", "Tab", "5",
                       "Tab", "Space", "Tab", "Tab", "Tab"});
    mwt.start();
    dim.exec();
    REQUIRE(dim.imageSizeAltered() == false);
    REQUIRE(dim.imageColoursAltered() == false);
    REQUIRE(dim.getAlteredImage().volume().width() == 100);
    REQUIRE(dim.getAlteredImage().volume().height() == 50);
    REQUIRE(dim.getAlteredImage().volume().depth() == 1);
    // width/height both altered as aspect ratio changes slightly due to
    // integer pixel count, so pixel width is not exactly preserved
    REQUIRE(std::abs(dim.getVoxelSize().width() - 1.0) < 0.02);
    REQUIRE(std::abs(dim.getVoxelSize().height() - 1.0) < 0.02);
    REQUIRE(dim.getVoxelSize().depth() == dbl_approx(1.0));
  }
  SECTION("select colours pressed, then reset colours") {
    REQUIRE(dim.getVoxelSize().width() == dbl_approx(1.0));
    REQUIRE(dim.getVoxelSize().height() == dbl_approx(1.0));
    REQUIRE(dim.getVoxelSize().depth() == dbl_approx(1.0));
    REQUIRE(dim.imageSizeAltered() == false);
    REQUIRE(dim.imageColoursAltered() == false);
    REQUIRE(dim.getAlteredImage().volume().width() == 100);
    REQUIRE(dim.getAlteredImage().volume().height() == 50);
    REQUIRE(dim.getAlteredImage().volume().depth() == 1);
    mwt.addUserAction({"Tab", "Tab", "Tab", "Tab", "Tab", "Tab", "Tab", "Tab",
                       "Space", "Space", "Tab"});
    mwt.start();
    dim.exec();
    REQUIRE(dim.getVoxelSize().width() == dbl_approx(1.0));
    REQUIRE(dim.getVoxelSize().height() == dbl_approx(1.0));
    REQUIRE(dim.getVoxelSize().depth() == dbl_approx(1.0));
    REQUIRE(dim.imageSizeAltered() == false);
    REQUIRE(dim.imageColoursAltered() == false);
    REQUIRE(dim.getAlteredImage().volume().width() == 100);
    REQUIRE(dim.getAlteredImage().volume().height() == 50);
    REQUIRE(dim.getAlteredImage().volume().depth() == 1);
  }
  SECTION("select colours pressed, then one chosen") {
    dim.show();
    REQUIRE(dim.getVoxelSize().width() == dbl_approx(1.0));
    REQUIRE(dim.getVoxelSize().height() == dbl_approx(1.0));
    REQUIRE(dim.getVoxelSize().depth() == dbl_approx(1.0));
    REQUIRE(dim.imageSizeAltered() == false);
    REQUIRE(dim.imageColoursAltered() == false);
    REQUIRE(dim.getAlteredImage().volume().width() == 100);
    REQUIRE(dim.getAlteredImage().volume().height() == 50);
    REQUIRE(dim.getAlteredImage().volume().depth() == 1);
    REQUIRE(dim.getAlteredImage()[0].colorCount() == 3);
    REQUIRE(okButton->isEnabled() == true);
    sendMouseClick(btnSelectColours);
    // ok/cancel disabled while selecting colours
    REQUIRE(okButton->isEnabled() == false);
    sendMouseClick(lblImage, QPoint(3, 3));
    REQUIRE(okButton->isEnabled() == false);
    sendMouseClick(btnApplyColours);
    REQUIRE(okButton->isEnabled() == true);
    REQUIRE(dim.getVoxelSize().width() == dbl_approx(1.0));
    REQUIRE(dim.getVoxelSize().height() == dbl_approx(1.0));
    REQUIRE(dim.getVoxelSize().depth() == dbl_approx(1.0));
    REQUIRE(dim.imageSizeAltered() == false);
    REQUIRE(dim.imageColoursAltered() == true);
    REQUIRE(dim.getAlteredImage().volume().width() == 100);
    REQUIRE(dim.getAlteredImage().volume().height() == 50);
    REQUIRE(dim.getAlteredImage().volume().depth() == 1);
    REQUIRE(dim.getAlteredImage()[0].colorCount() == 1);
  }
}

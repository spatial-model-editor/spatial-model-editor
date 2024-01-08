#include "catch_wrapper.hpp"
#include "dialoggeometryimage.hpp"
#include "model_test_utils.hpp"
#include "qlabelmousetracker.hpp"
#include "qt_test_utils.hpp"
#include "sme/model.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>

using namespace sme::test;

struct DialogGeometryImageWidgets {
  explicit DialogGeometryImageWidgets(const DialogGeometryImage *dialog) {
    GET_DIALOG_WIDGET(QLabel, lblImage);
    GET_DIALOG_WIDGET(QSlider, slideZIndex);
    GET_DIALOG_WIDGET(QLineEdit, txtImageWidth);
    GET_DIALOG_WIDGET(QComboBox, cmbUnitsWidth);
    GET_DIALOG_WIDGET(QLineEdit, txtImageHeight);
    GET_DIALOG_WIDGET(QLineEdit, txtImageDepth);
    GET_DIALOG_WIDGET(QCheckBox, chkKeepAspectRatio);
    GET_DIALOG_WIDGET(QSpinBox, spinPixelsX);
    GET_DIALOG_WIDGET(QSpinBox, spinPixelsY);
    GET_DIALOG_WIDGET(QPushButton, btnResetPixels);
    GET_DIALOG_WIDGET(QLabel, lblColours);
    GET_DIALOG_WIDGET(QPushButton, btnSelectColours);
    GET_DIALOG_WIDGET(QPushButton, btnApplyColours);
    GET_DIALOG_WIDGET(QPushButton, btnResetColours);
    GET_DIALOG_WIDGET(QLabel, lblPixelSize);
    GET_DIALOG_WIDGET(QDialogButtonBox, buttonBox);
  }
  QLabel *lblImage;
  QSlider *slideZIndex;
  QLineEdit *txtImageWidth;
  QComboBox *cmbUnitsWidth;
  QLineEdit *txtImageHeight;
  QLineEdit *txtImageDepth;
  QCheckBox *chkKeepAspectRatio;
  QSpinBox *spinPixelsX;
  QSpinBox *spinPixelsY;
  QPushButton *btnResetPixels;
  QLabel *lblColours;
  QPushButton *btnSelectColours;
  QPushButton *btnApplyColours;
  QPushButton *btnResetColours;
  QLabel *lblPixelSize;
  QDialogButtonBox *buttonBox;
};

TEST_CASE("DialogGeometryImage",
          "[gui/dialogs/geometryimage][gui/dialogs][gui][geometryimage]") {
  auto m{getExampleModel(Mod::ABtoC)};
  auto modelUnits = m.getUnits();
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
  SECTION("Use m for length units, initial voxel size 1^3") {
    modelUnits.setLengthIndex(0);
    REQUIRE(modelUnits.getLength().name == "m");
    DialogGeometryImage dia(img, {1.0, 1.0, 1.0}, modelUnits);
    dia.show();
    waitFor(&dia);
    DialogGeometryImageWidgets widgets(&dia);
    REQUIRE(dia.getVoxelSize().width() == dbl_approx(1.0));
    REQUIRE(dia.getVoxelSize().height() == dbl_approx(1.0));
    REQUIRE(dia.getVoxelSize().depth() == dbl_approx(1.0));
    SECTION("user sets image width to 1, same units, voxel width -> 0.01") {
      sendKeyEvents(widgets.txtImageWidth,
                    {"Backspace", "Backspace", "Backspace", "1", "Enter"});
      REQUIRE(dia.getVoxelSize().width() == dbl_approx(0.01));
      REQUIRE(dia.getVoxelSize().height() == dbl_approx(1.0));
      REQUIRE(dia.getVoxelSize().depth() == dbl_approx(1.0));
      REQUIRE(dia.imageSizeAltered() == false);
      REQUIRE(dia.imageColoursAltered() == false);
    }
    SECTION("user sets image width to 1e-8, voxel width -> 1e-10") {
      sendKeyEvents(
          widgets.txtImageWidth,
          {"Backspace", "Backspace", "Backspace", "1", "e", "-", "8", "Enter"});
      REQUIRE(dia.getVoxelSize().width() == dbl_approx(1e-10));
      REQUIRE(dia.getVoxelSize().height() == dbl_approx(1.0));
      REQUIRE(dia.getVoxelSize().depth() == dbl_approx(1.0));
      REQUIRE(dia.imageSizeAltered() == false);
      REQUIRE(dia.imageColoursAltered() == false);
    }
    SECTION("user sets image height to 10, voxel height -> 0.2") {
      sendKeyEvents(widgets.txtImageHeight,
                    {"Backspace", "Backspace", "Backspace", "1", "0", "Enter"});
      REQUIRE(dia.getVoxelSize().width() == dbl_approx(1.0));
      REQUIRE(dia.getVoxelSize().height() == dbl_approx(0.2));
      REQUIRE(dia.getVoxelSize().depth() == dbl_approx(1.0));
      REQUIRE(dia.imageSizeAltered() == false);
      REQUIRE(dia.imageColoursAltered() == false);
    }
    SECTION("user sets image height to 10, units to dm, voxel height -> 0.02, "
            "others /=10") {
      sendKeyEvents(widgets.txtImageHeight,
                    {"Backspace", "Backspace", "Backspace", "1", "0", "Enter"});
      REQUIRE(dia.getVoxelSize().width() == dbl_approx(1.0));
      REQUIRE(dia.getVoxelSize().height() == dbl_approx(0.2));
      REQUIRE(dia.getVoxelSize().depth() == dbl_approx(1.0));
      widgets.cmbUnitsWidth->setFocus();
      sendKeyEvents(widgets.cmbUnitsWidth, {"Down"});
      REQUIRE(widgets.cmbUnitsWidth->currentText() == "dm");
      REQUIRE(dia.getVoxelSize().width() == dbl_approx(0.1));
      REQUIRE(dia.getVoxelSize().height() == dbl_approx(0.02));
      REQUIRE(dia.getVoxelSize().depth() == dbl_approx(0.1));
      REQUIRE(dia.imageSizeAltered() == false);
      REQUIRE(dia.imageColoursAltered() == false);
    }
    SECTION("user sets image height to 10, units to cm, voxel height -> 0.002, "
            "others /=100") {
      sendKeyEvents(widgets.txtImageHeight,
                    {"Backspace", "Backspace", "Backspace", "1", "0", "Enter"});
      REQUIRE(dia.getVoxelSize().width() == dbl_approx(1.0));
      REQUIRE(dia.getVoxelSize().height() == dbl_approx(0.2));
      REQUIRE(dia.getVoxelSize().depth() == dbl_approx(1.0));
      widgets.cmbUnitsWidth->setFocus();
      sendKeyEvents(widgets.cmbUnitsWidth, {"Down", "Down"});
      REQUIRE(widgets.cmbUnitsWidth->currentText() == "cm");
      REQUIRE(dia.getVoxelSize().width() == dbl_approx(0.01));
      REQUIRE(dia.getVoxelSize().height() == dbl_approx(0.002));
      REQUIRE(dia.getVoxelSize().depth() == dbl_approx(0.01));
      REQUIRE(dia.imageSizeAltered() == false);
      REQUIRE(dia.imageColoursAltered() == false);
    }
    SECTION("user sets image width to 9 & units to dm, then image height 10, "
            "units um") {
      sendKeyEvents(widgets.txtImageWidth,
                    {"Backspace", "Backspace", "Backspace", "9", "Enter"});
      widgets.cmbUnitsWidth->setFocus();
      sendKeyEvents(widgets.cmbUnitsWidth, {"Down", "Down", "Down", "Down"});
      REQUIRE(widgets.cmbUnitsWidth->currentText() == "um");
      sendKeyEvents(widgets.txtImageHeight,
                    {"Backspace", "Backspace", "Backspace", "1", "0", "Enter"});
      REQUIRE(dia.getVoxelSize().width() == dbl_approx(0.09 * 1e-6));
      REQUIRE(dia.getVoxelSize().height() == dbl_approx(0.2 * 1e-6));
      REQUIRE(dia.getVoxelSize().depth() == dbl_approx(1e-6));
      REQUIRE(dia.imageSizeAltered() == false);
      REQUIRE(dia.imageColoursAltered() == false);
    }
    SECTION("user sets depth to 4, no change of units") {
      sendKeyEvents(widgets.txtImageDepth,
                    {"Backspace", "Backspace", "Backspace", "4", "Enter"});
      REQUIRE(dia.getVoxelSize().width() == dbl_approx(1.0));
      REQUIRE(dia.getVoxelSize().height() == dbl_approx(1.0));
      REQUIRE(dia.getVoxelSize().depth() == dbl_approx(4.0));
      REQUIRE(dia.imageSizeAltered() == false);
      REQUIRE(dia.imageColoursAltered() == false);
    }
    SECTION("x pixels reduced from 100 to 98") {
      REQUIRE(dia.getVoxelSize().width() == dbl_approx(1.0));
      REQUIRE(dia.getVoxelSize().height() == dbl_approx(1.0));
      REQUIRE(dia.getVoxelSize().depth() == dbl_approx(1.0));
      REQUIRE(dia.imageSizeAltered() == false);
      REQUIRE(dia.imageColoursAltered() == false);
      REQUIRE(dia.getAlteredImage().volume().width() == 100);
      REQUIRE(dia.getAlteredImage().volume().height() == 50);
      REQUIRE(dia.getAlteredImage().volume().depth() == 1);
      REQUIRE(widgets.spinPixelsX->value() == 100);
      sendKeyEvents(widgets.spinPixelsX, {"Down", "Down"});
      REQUIRE(widgets.spinPixelsX->value() == 98);
      REQUIRE(dia.imageSizeAltered() == true);
      REQUIRE(dia.imageColoursAltered() == false);
      REQUIRE(dia.getVoxelSize().width() == dbl_approx(1.0 / 0.98));
      REQUIRE(dia.getVoxelSize().height() == dbl_approx(1.0 / 0.98));
      REQUIRE(dia.getVoxelSize().depth() == dbl_approx(1.0));
      REQUIRE(dia.getAlteredImage().volume().width() == 98);
      REQUIRE(dia.getAlteredImage().volume().height() == 49);
      REQUIRE(dia.getAlteredImage().volume().depth() == 1);
    }
    SECTION("y pixels reduced from 50 to 5") {
      REQUIRE(dia.getVoxelSize().width() == dbl_approx(1.0));
      REQUIRE(dia.getVoxelSize().height() == dbl_approx(1.0));
      REQUIRE(dia.getVoxelSize().depth() == dbl_approx(1.0));
      REQUIRE(dia.imageSizeAltered() == false);
      REQUIRE(dia.imageColoursAltered() == false);
      REQUIRE(dia.getAlteredImage().volume().width() == 100);
      REQUIRE(dia.getAlteredImage().volume().height() == 50);
      REQUIRE(dia.getAlteredImage().volume().depth() == 1);
      REQUIRE(widgets.spinPixelsY->value() == 50);
      widgets.spinPixelsY->setValue(5);
      REQUIRE(widgets.spinPixelsY->value() == 5);
      REQUIRE(dia.imageSizeAltered() == true);
      REQUIRE(dia.imageColoursAltered() == false);
      REQUIRE(dia.getVoxelSize().width() == dbl_approx(10.0));
      REQUIRE(dia.getVoxelSize().height() == dbl_approx(10.0));
      REQUIRE(dia.getVoxelSize().depth() == dbl_approx(1.0));
      REQUIRE(dia.getAlteredImage().volume().width() == 10);
      REQUIRE(dia.getAlteredImage().volume().height() == 5);
      REQUIRE(dia.getAlteredImage().volume().depth() == 1);
    }
    SECTION("x pixels reduced, y pixels reduced, reset pixels pressed") {
      REQUIRE(dia.getVoxelSize().width() == dbl_approx(1.0));
      REQUIRE(dia.getVoxelSize().height() == dbl_approx(1.0));
      REQUIRE(dia.getVoxelSize().depth() == dbl_approx(1.0));
      REQUIRE(dia.imageSizeAltered() == false);
      REQUIRE(dia.imageColoursAltered() == false);
      REQUIRE(dia.getAlteredImage().volume().width() == 100);
      REQUIRE(dia.getAlteredImage().volume().height() == 50);
      REQUIRE(dia.getAlteredImage().volume().depth() == 1);
      REQUIRE(widgets.chkKeepAspectRatio->isChecked());
      // 100->98 x pixels, implies 50->49 y pixels
      REQUIRE(widgets.spinPixelsX->value() == 100);
      widgets.spinPixelsX->setValue(98);
      REQUIRE(widgets.spinPixelsX->value() == 98);
      REQUIRE(dia.getVoxelSize().width() == dbl_approx(1.0 / 0.98));
      REQUIRE(dia.getVoxelSize().height() == dbl_approx(1.0 / 0.98));
      REQUIRE(dia.getVoxelSize().depth() == dbl_approx(1.0));
      REQUIRE(widgets.spinPixelsY->value() == 49);
      // 98->99 x pixels, 49->50 y pixels
      REQUIRE(widgets.spinPixelsX->value() == 98);
      widgets.spinPixelsX->setValue(99);
      REQUIRE(widgets.spinPixelsX->value() == 99);
      REQUIRE(dia.getVoxelSize().width() == dbl_approx(1.0 / 0.99));
      REQUIRE(dia.getVoxelSize().height() == dbl_approx(1.0));
      REQUIRE(dia.getVoxelSize().depth() == dbl_approx(1.0));
      REQUIRE(widgets.spinPixelsY->value() == 50);
      // 99->100 x pixels, 50->50 y pixels
      REQUIRE(widgets.spinPixelsX->value() == 99);
      widgets.spinPixelsX->setValue(100);
      REQUIRE(widgets.spinPixelsX->value() == 100);
      REQUIRE(dia.getVoxelSize().width() == dbl_approx(1.0));
      REQUIRE(dia.getVoxelSize().height() == dbl_approx(1.0));
      REQUIRE(dia.getVoxelSize().depth() == dbl_approx(1.0));
      REQUIRE(widgets.spinPixelsY->value() == 50);
      widgets.spinPixelsY->setValue(5);
      REQUIRE(widgets.spinPixelsY->value() == 5);
      REQUIRE(dia.getVoxelSize().width() == dbl_approx(10.0));
      REQUIRE(dia.getVoxelSize().height() == dbl_approx(10.0));
      REQUIRE(dia.getVoxelSize().depth() == dbl_approx(1.0));
      sendMouseClick(widgets.btnResetPixels);
      REQUIRE(dia.imageSizeAltered() == false);
      REQUIRE(dia.imageColoursAltered() == false);
      REQUIRE(dia.getAlteredImage().volume().width() == 100);
      REQUIRE(dia.getAlteredImage().volume().height() == 50);
      REQUIRE(dia.getAlteredImage().volume().depth() == 1);
      REQUIRE(dia.getVoxelSize().width() == dbl_approx(1.0));
      REQUIRE(dia.getVoxelSize().height() == dbl_approx(1.0));
      REQUIRE(dia.getVoxelSize().depth() == dbl_approx(1.0));
    }
    SECTION("x pixels reduced, y pixels reduced, reset pixels pressed, "
            "maintain aspect ratio off") {
      REQUIRE(dia.getVoxelSize().width() == dbl_approx(1.0));
      REQUIRE(dia.getVoxelSize().height() == dbl_approx(1.0));
      REQUIRE(dia.getVoxelSize().depth() == dbl_approx(1.0));
      REQUIRE(dia.imageSizeAltered() == false);
      REQUIRE(dia.imageColoursAltered() == false);
      REQUIRE(dia.getAlteredImage().volume().width() == 100);
      REQUIRE(dia.getAlteredImage().volume().height() == 50);
      REQUIRE(dia.getAlteredImage().volume().depth() == 1);
      sendMouseClick(widgets.chkKeepAspectRatio);
      REQUIRE(widgets.chkKeepAspectRatio->isChecked() == false);
      // 100->98 x pixels, y unchanged
      REQUIRE(widgets.spinPixelsX->value() == 100);
      widgets.spinPixelsX->setValue(98);
      REQUIRE(widgets.spinPixelsX->value() == 98);
      REQUIRE(dia.getVoxelSize().width() == dbl_approx(1.0 / 0.98));
      REQUIRE(dia.getVoxelSize().height() == dbl_approx(1.0));
      REQUIRE(dia.getVoxelSize().depth() == dbl_approx(1.0));
      REQUIRE(widgets.spinPixelsY->value() == 50);
      // 98->99 x pixels, y unchanged
      REQUIRE(widgets.spinPixelsX->value() == 98);
      widgets.spinPixelsX->setValue(99);
      REQUIRE(widgets.spinPixelsX->value() == 99);
      REQUIRE(dia.getVoxelSize().width() == dbl_approx(1.0 / 0.99));
      REQUIRE(dia.getVoxelSize().height() == dbl_approx(1.0));
      REQUIRE(dia.getVoxelSize().depth() == dbl_approx(1.0));
      REQUIRE(widgets.spinPixelsY->value() == 50);
      // 99->107 x pixels, y unchanged
      REQUIRE(widgets.spinPixelsX->value() == 99);
      widgets.spinPixelsX->setValue(107);
      REQUIRE(widgets.spinPixelsX->value() == 107);
      REQUIRE(dia.getVoxelSize().width() == dbl_approx(1.0 / 1.07));
      REQUIRE(dia.getVoxelSize().height() == dbl_approx(1.0));
      REQUIRE(dia.getVoxelSize().depth() == dbl_approx(1.0));
      REQUIRE(widgets.spinPixelsY->value() == 50);
      // 50 -> 5 y pixels, x unchanged
      widgets.spinPixelsY->setValue(5);
      REQUIRE(widgets.spinPixelsY->value() == 5);
      REQUIRE(dia.getVoxelSize().width() == dbl_approx(1.0 / 1.07));
      REQUIRE(dia.getVoxelSize().height() == dbl_approx(10.0));
      REQUIRE(dia.getVoxelSize().depth() == dbl_approx(1.0));
      sendMouseClick(widgets.btnResetPixels);
      REQUIRE(dia.imageSizeAltered() == false);
      REQUIRE(dia.imageColoursAltered() == false);
      REQUIRE(dia.getAlteredImage().volume().width() == 100);
      REQUIRE(dia.getAlteredImage().volume().height() == 50);
      REQUIRE(dia.getAlteredImage().volume().depth() == 1);
      REQUIRE(dia.getVoxelSize().width() == dbl_approx(1.0));
      REQUIRE(dia.getVoxelSize().height() == dbl_approx(1.0));
      REQUIRE(dia.getVoxelSize().depth() == dbl_approx(1.0));
    }
    SECTION("select colours pressed, then reset colours") {
      REQUIRE(dia.getVoxelSize().width() == dbl_approx(1.0));
      REQUIRE(dia.getVoxelSize().height() == dbl_approx(1.0));
      REQUIRE(dia.getVoxelSize().depth() == dbl_approx(1.0));
      REQUIRE(dia.imageSizeAltered() == false);
      REQUIRE(dia.imageColoursAltered() == false);
      REQUIRE(dia.getAlteredImage().volume().width() == 100);
      REQUIRE(dia.getAlteredImage().volume().height() == 50);
      REQUIRE(dia.getAlteredImage().volume().depth() == 1);
      REQUIRE(widgets.txtImageWidth->isEnabled() == true);
      REQUIRE(widgets.buttonBox->button(QDialogButtonBox::Ok)->isEnabled() ==
              true);
      sendMouseClick(widgets.btnSelectColours);
      REQUIRE(widgets.txtImageWidth->isEnabled() == false);
      REQUIRE(widgets.buttonBox->button(QDialogButtonBox::Ok)->isEnabled() ==
              false);
      sendMouseClick(widgets.btnResetColours);
      REQUIRE(widgets.txtImageWidth->isEnabled() == true);
      REQUIRE(widgets.buttonBox->button(QDialogButtonBox::Ok)->isEnabled() ==
              true);
      REQUIRE(dia.getVoxelSize().width() == dbl_approx(1.0));
      REQUIRE(dia.getVoxelSize().height() == dbl_approx(1.0));
      REQUIRE(dia.getVoxelSize().depth() == dbl_approx(1.0));
      REQUIRE(dia.imageSizeAltered() == false);
      REQUIRE(dia.imageColoursAltered() == false);
      REQUIRE(dia.getAlteredImage().volume().width() == 100);
      REQUIRE(dia.getAlteredImage().volume().height() == 50);
      REQUIRE(dia.getAlteredImage().volume().depth() == 1);
    }
    SECTION("select colours pressed, then one chosen") {
      REQUIRE(dia.getVoxelSize().width() == dbl_approx(1.0));
      REQUIRE(dia.getVoxelSize().height() == dbl_approx(1.0));
      REQUIRE(dia.getVoxelSize().depth() == dbl_approx(1.0));
      REQUIRE(dia.imageSizeAltered() == false);
      REQUIRE(dia.imageColoursAltered() == false);
      REQUIRE(dia.getAlteredImage().volume().width() == 100);
      REQUIRE(dia.getAlteredImage().volume().height() == 50);
      REQUIRE(dia.getAlteredImage().volume().depth() == 1);
      REQUIRE(dia.getAlteredImage()[0].colorCount() == 3);
      REQUIRE(widgets.txtImageWidth->isEnabled() == true);
      REQUIRE(widgets.buttonBox->button(QDialogButtonBox::Ok)->isEnabled() ==
              true);
      REQUIRE(widgets.btnApplyColours->isEnabled() == false);
      sendMouseClick(widgets.btnSelectColours);
      REQUIRE(widgets.txtImageWidth->isEnabled() == false);
      REQUIRE(widgets.buttonBox->button(QDialogButtonBox::Ok)->isEnabled() ==
              false);
      REQUIRE(widgets.btnApplyColours->isEnabled() == false);
      sendMouseClick(widgets.lblImage, QPoint(3, 3));
      REQUIRE(widgets.btnApplyColours->isEnabled() == true);
      sendMouseClick(widgets.btnApplyColours);
      REQUIRE(widgets.txtImageWidth->isEnabled() == true);
      REQUIRE(widgets.buttonBox->button(QDialogButtonBox::Ok)->isEnabled() ==
              true);
      REQUIRE(widgets.btnApplyColours->isEnabled() == false);
      REQUIRE(dia.getVoxelSize().width() == dbl_approx(1.0));
      REQUIRE(dia.getVoxelSize().height() == dbl_approx(1.0));
      REQUIRE(dia.getVoxelSize().depth() == dbl_approx(1.0));
      REQUIRE(dia.imageSizeAltered() == false);
      REQUIRE(dia.imageColoursAltered() == true);
      REQUIRE(dia.getAlteredImage().volume().width() == 100);
      REQUIRE(dia.getAlteredImage().volume().height() == 50);
      REQUIRE(dia.getAlteredImage().volume().depth() == 1);
      REQUIRE(dia.getAlteredImage()[0].colorCount() == 1);
    }
  }
  SECTION("Use mm for length units, initial voxel size 22x22x1") {
    modelUnits.setLengthIndex(3);
    REQUIRE(modelUnits.getLength().name == "mm");
    DialogGeometryImage dia(img, {22.0, 22.0, 1.0}, modelUnits);
    dia.show();
    waitFor(&dia);
    DialogGeometryImageWidgets widgets(&dia);
    REQUIRE(dia.getVoxelSize().width() == dbl_approx(22.0));
    REQUIRE(dia.getVoxelSize().height() == dbl_approx(22.0));
    REQUIRE(dia.getVoxelSize().depth() == dbl_approx(1.0));
    SECTION("user sets image width = 1, units = m") {
      widgets.txtImageWidth->setFocus();
      widgets.txtImageWidth->clear();
      sendKeyEvents(widgets.txtImageWidth, {"1", "Tab"});
      REQUIRE(widgets.cmbUnitsWidth->currentText() == "mm");
      wait(100);
      REQUIRE(dia.getVoxelSize().width() == dbl_approx(0.01));
      REQUIRE(dia.getVoxelSize().height() == dbl_approx(22.0));
      REQUIRE(dia.getVoxelSize().depth() == dbl_approx(1.0));
      widgets.cmbUnitsWidth->setFocus();
      sendKeyEvents(widgets.cmbUnitsWidth, {"Up", "Up", "Up"});
      REQUIRE(widgets.cmbUnitsWidth->currentText() == "m");
      REQUIRE(dia.getVoxelSize().width() == dbl_approx(10.0));
      REQUIRE(dia.getVoxelSize().height() == dbl_approx(22e3));
      REQUIRE(dia.getVoxelSize().depth() == dbl_approx(1e3));
      REQUIRE(dia.imageSizeAltered() == false);
      REQUIRE(dia.imageColoursAltered() == false);
    }
  }
}

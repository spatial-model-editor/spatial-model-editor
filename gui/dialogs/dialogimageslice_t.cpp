#include "catch_wrapper.hpp"
#include "dialogimageslice.hpp"
#include "qlabelmousetracker.hpp"
#include "qlabelslice.hpp"
#include "qt_test_utils.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QFile>
#include <QLabel>

using namespace sme::test;

struct DialogImageSliceWidgets {
  explicit DialogImageSliceWidgets(const DialogImageSlice *dialog) {
    GET_DIALOG_WIDGET(QComboBox, cmbSliceType);
    GET_DIALOG_WIDGET(QCheckBox, chkAspectRatio);
    GET_DIALOG_WIDGET(QCheckBox, chkSmoothInterpolation);
    GET_DIALOG_WIDGET(QLabel, lblSlice);
    GET_DIALOG_WIDGET(QLabel, lblImage);
    GET_DIALOG_WIDGET(QLabel, lblMouseLocation);
  }
  QComboBox *cmbSliceType;
  QCheckBox *chkAspectRatio;
  QCheckBox *chkSmoothInterpolation;
  QLabel *lblSlice;
  QLabel *lblImage;
  QLabel *lblMouseLocation;
};

TEST_CASE("DialogImageSlice",
          "[gui/dialogs/imageslice][gui/dialogs][gui][imageslice]") {
  sme::common::ImageStack imgGeometry({100, 50, 1},
                                      QImage::Format_ARGB32_Premultiplied);
  QVector<sme::common::ImageStack> imgs(5, imgGeometry);
  QRgb col1 = 0xffa34f6c;
  QRgb col2 = 0xff123456;
  for (auto &img : imgs) {
    img.fill(col1);
  }
  imgs[3].fill(col2);
  QVector<double> time{0, 1, 2, 3, 4};
  DialogImageSlice dia(imgGeometry, imgs, time, false);
  dia.show();
  DialogImageSliceWidgets widgets(&dia);
  SECTION("mouse moves, text changes") {
    QImage slice = dia.getSlicedImage();
    REQUIRE(slice.width() == imgs.size());
    REQUIRE(slice.height() == imgs[0][0].height());
    REQUIRE(slice.pixel(1, 35) == col1);
    REQUIRE(slice.pixel(2, 49) == col1);
    REQUIRE(slice.pixel(3, 12) == col2);
    REQUIRE(slice.pixel(4, 0) == col1);
    auto oldText{widgets.lblMouseLocation->text()};
    sendMouseMove(widgets.lblSlice, {10, 10});
    auto newText{widgets.lblMouseLocation->text()};
    REQUIRE(oldText != newText);
    oldText = newText;
    sendMouseMove(widgets.lblSlice, {20, 20});
    newText = widgets.lblMouseLocation->text();
    REQUIRE(oldText != newText);
    oldText = newText;
    sendMouseMove(widgets.lblSlice, {1, 1});
    newText = widgets.lblMouseLocation->text();
    REQUIRE(oldText != newText);
    oldText = newText;
    sendMouseMove(widgets.lblImage, {1, 1});
    sendMouseMove(widgets.lblImage, {40, 32});
    newText = widgets.lblMouseLocation->text();
    REQUIRE(oldText != newText);
  }
  SECTION("user sets slice to horizontal") {
    sendKeyEvents(widgets.cmbSliceType, {"Up"});
    QImage slice = dia.getSlicedImage();
    REQUIRE(slice.width() == imgs.size());
    REQUIRE(slice.height() == imgs[0][0].width());
    REQUIRE(slice.pixel(1, 75) == col1);
    REQUIRE(slice.pixel(2, 28) == col1);
    REQUIRE(slice.pixel(3, 99) == col2);
    REQUIRE(slice.pixel(4, 0) == col1);
  }
  SECTION("user sets slice to custom: default is diagonal") {
    sendKeyEvents(widgets.cmbSliceType, {"Down"});
    QImage slice = dia.getSlicedImage();
    REQUIRE(slice.width() == imgs.size());
    REQUIRE(slice.height() ==
            std::max(imgs[0][0].height(), imgs[0][0].width()));
    REQUIRE(slice.pixel(1, 75) == col1);
    REQUIRE(slice.pixel(2, 28) == col1);
    REQUIRE(slice.pixel(3, 99) == col2);
    REQUIRE(slice.pixel(4, 0) == col1);
  }
  SECTION("user clicks save image, then cancel") {
    ModalWidgetTimer mwt;
    mwt.addUserAction({"Esc"});
    mwt.start();
    sendKeyEvents(&dia, {"Enter"});
    REQUIRE(mwt.getResult() == "QFileDialog::AcceptSave");
  }
  SECTION("user clicks save image, then enters filename") {
    ModalWidgetTimer mwt;
    mwt.addUserAction({"t", "m", "p", "d", "s", "l", "i", "c", "e"});
    mwt.start();
    sendKeyEvents(&dia, {"Enter"});
    REQUIRE(mwt.getResult() == "QFileDialog::AcceptSave");
    QImage img("tmpdslice.png");
    REQUIRE(img.width() == imgs.size());
    REQUIRE(img.height() == imgs[0][0].height());
  }
}

#include "catch_wrapper.hpp"
#include "dialogimageslice.hpp"
#include "qlabelmousetracker.hpp"
#include "qlabelslice.hpp"
#include "qt_test_utils.hpp"
#include <QFile>

SCENARIO("DialogImageSlice",
         "[gui/dialogs/imageslice][gui/dialogs][gui][imageslice]") {
  GIVEN("5 100x50 images") {
    QImage imgGeometry(100, 50, QImage::Format_ARGB32_Premultiplied);
    QVector<QImage> imgs(5,
                         QImage(100, 50, QImage::Format_ARGB32_Premultiplied));
    QRgb col1 = 0xffa34f6c;
    QRgb col2 = 0xff123456;
    for (auto &img : imgs) {
      img.fill(col1);
    }
    imgs[3].fill(col2);
    QVector<double> time{0, 1, 2, 3, 4};
    DialogImageSlice dia(imgGeometry, imgs, time, false);
    auto *lblSlice{dia.findChild<QLabelSlice *>("lblSlice")};
    REQUIRE(lblSlice != nullptr);
    auto *lblImage{dia.findChild<QLabelMouseTracker *>("lblImage")};
    REQUIRE(lblImage != nullptr);
    auto *lblMouseLocation{dia.findChild<QLabel *>("lblMouseLocation")};
    REQUIRE(lblMouseLocation != nullptr);
    ModalWidgetTimer mwt;
    WHEN("mouse moves, text changes") {
      QImage slice = dia.getSlicedImage();
      REQUIRE(slice.width() == imgs.size());
      REQUIRE(slice.height() == imgs[0].height());
      REQUIRE(slice.pixel(1, 35) == col1);
      REQUIRE(slice.pixel(2, 49) == col1);
      REQUIRE(slice.pixel(3, 12) == col2);
      REQUIRE(slice.pixel(4, 0) == col1);
      auto oldText{lblMouseLocation->text()};
      sendMouseMove(lblSlice, {10, 10});
      auto newText{lblMouseLocation->text()};
      REQUIRE(oldText != newText);
      oldText = newText;
      sendMouseMove(lblSlice, {20, 20});
      newText = lblMouseLocation->text();
      REQUIRE(oldText != newText);
      oldText = newText;
      sendMouseMove(lblSlice, {1, 1});
      newText = lblMouseLocation->text();
      REQUIRE(oldText != newText);
      oldText = newText;
      sendMouseMove(lblImage, {1, 1});
      sendMouseMove(lblImage, {40, 32});
      newText = lblMouseLocation->text();
      REQUIRE(oldText != newText);
    }
    WHEN("user sets slice to horizontal") {
      mwt.addUserAction({"Up"});
      mwt.start();
      dia.exec();
      QImage slice = dia.getSlicedImage();
      REQUIRE(slice.width() == imgs.size());
      REQUIRE(slice.height() == imgs[0].width());
      REQUIRE(slice.pixel(1, 75) == col1);
      REQUIRE(slice.pixel(2, 28) == col1);
      REQUIRE(slice.pixel(3, 99) == col2);
      REQUIRE(slice.pixel(4, 0) == col1);
    }
    WHEN("user sets slice to custom: default is diagonal") {
      mwt.addUserAction({"Down"});
      mwt.start();
      dia.exec();
      QImage slice = dia.getSlicedImage();
      REQUIRE(slice.width() == imgs.size());
      REQUIRE(slice.height() == std::max(imgs[0].height(), imgs[0].width()));
      REQUIRE(slice.pixel(1, 75) == col1);
      REQUIRE(slice.pixel(2, 28) == col1);
      REQUIRE(slice.pixel(3, 99) == col2);
      REQUIRE(slice.pixel(4, 0) == col1);
    }
    WHEN("user clicks save image, then cancel") {
      ModalWidgetTimer mwt2;
      mwt.addUserAction({"Enter"}, true, &mwt2);
      mwt2.addUserAction({"Esc"});
      mwt.start();
      dia.exec();
      REQUIRE(mwt2.getResult() == "QFileDialog::AcceptSave");
    }
    WHEN("user clicks save image, then enters filename") {
      ModalWidgetTimer mwt2;
      mwt.addUserAction({"Enter"}, true, &mwt2);
      mwt2.addUserAction({"x", "y", "z"});
      mwt.start();
      dia.exec();
      REQUIRE(mwt2.getResult() == "QFileDialog::AcceptSave");
      QImage img("xyz.png");
      REQUIRE(img.width() == imgs.size());
      REQUIRE(img.height() == imgs[0].height());
    }
  }
}

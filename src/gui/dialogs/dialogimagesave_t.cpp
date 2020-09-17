#include "catch_wrapper.hpp"
#include "dialogimagesave.hpp"
#include "qt_test_utils.hpp"
#include <QFile>

SCENARIO("DialogImageSave",
         "[gui/dialogs/imagesave][gui/dialogs][gui][imagesave]") {
  GIVEN("5 100x50 images") {
    QImage imgGeometry(100, 50, QImage::Format_ARGB32_Premultiplied);
    QVector<QImage> imgs(5,
                         QImage(100, 50, QImage::Format_ARGB32_Premultiplied));
    QRgb col0 = 0xff121212;
    QRgb col1 = 0xffa34f6c;
    QRgb col2 = 0xff123456;
    QRgb col3 = 0xff003456;
    QRgb col4 = 0xff170f56;
    imgs[0].fill(col0);
    imgs[1].fill(col1);
    imgs[2].fill(col2);
    imgs[3].fill(col3);
    imgs[4].fill(col4);
    QVector<double> time{0, 1, 2, 3, 4};
    DialogImageSave dia(imgs, time, 2);
    ModalWidgetTimer mwt;
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
      REQUIRE(img.size() == imgs[0].size());
      REQUIRE(img.pixel(0, 0) == col2);
    }
    WHEN("user changes timepoint, clicks save image, then enters filename") {
      ModalWidgetTimer mwt2;
      mwt.addUserAction({"Tab", "Down", "Down", "Enter"}, true, &mwt2);
      mwt2.addUserAction({"x", "y", "z"});
      mwt.start();
      dia.exec();
      REQUIRE(mwt2.getResult() == "QFileDialog::AcceptSave");
      QImage img("xyz.png");
      REQUIRE(img.size() == imgs[0].size());
      REQUIRE(img.pixel(0, 0) == col4);
    }
    WHEN("user clicks on All timepoints, clicks save image, then enter") {
      ModalWidgetTimer mwt2;
      mwt.addUserAction({"Down", "Enter"}, true, &mwt2);
      mwt2.addUserAction({"Enter"});
      mwt.start();
      dia.exec();
      REQUIRE(mwt2.getResult() == "QFileDialog::AcceptOpen");
      QImage img0("img0.png");
      REQUIRE(img0.size() == imgs[0].size());
      REQUIRE(img0.pixel(0, 0) == col0);
      QImage img1("img1.png");
      REQUIRE(img1.size() == imgs[1].size());
      REQUIRE(img1.pixel(0, 0) == col1);
      QImage img2("img2.png");
      REQUIRE(img2.size() == imgs[2].size());
      REQUIRE(img2.pixel(0, 0) == col2);
      QImage img3("img3.png");
      REQUIRE(img3.size() == imgs[3].size());
      REQUIRE(img3.pixel(0, 0) == col3);
      QImage img4("img4.png");
      REQUIRE(img4.size() == imgs[4].size());
      REQUIRE(img4.pixel(0, 0) == col4);
    }
  }
}

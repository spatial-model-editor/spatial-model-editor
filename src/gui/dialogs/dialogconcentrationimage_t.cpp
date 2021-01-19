#include <QFile>
#include <numeric>
#include "catch_wrapper.hpp"
#include "dialogconcentrationimage.hpp"
#include "model.hpp"
#include "qt_test_utils.hpp"

SCENARIO(
    "DialogConcentrationImage",
    "[gui/dialogs/concentrationimage][gui/dialogs][gui][concentrationimage]") {
  GIVEN("3x3 compartment image, 4 pixel compartment") {
    sme::model::Model doc;
    QFile f(":/models/ABtoC.xml");
    f.open(QIODevice::ReadOnly);
    doc.importSBMLString(f.readAll().toStdString());
    auto sz = QSize(3, 3);
    auto origin = QPointF(0.0, 0.0);
    double width = 1.0;
    auto compartmentPoints = std::vector<QPoint>{QPoint(0, 0), QPoint(0, 1),
                                                 QPoint(0, 2), QPoint(1, 0)};
    auto compArrayIndices = std::vector<std::size_t>{6, 3, 0, 7};
    auto outsideArrayIndices = std::vector<std::size_t>{1, 2, 4, 5, 8};
    sme::model::SpeciesGeometry specGeom{sz, compartmentPoints, origin, width,
                                    doc.getUnits()};
    ModalWidgetTimer mwt;

    WHEN("empty concentration array: set concentration to 0") {
      DialogConcentrationImage dia({}, specGeom);
      for (const auto &a : dia.getConcentrationArray()) {
        REQUIRE(a == dbl_approx(0));
      }
    }
    WHEN("concentration array of 0's") {
      DialogConcentrationImage dia(std::vector<double>(9, 0.0), specGeom);
      for (const auto &a : dia.getConcentrationArray()) {
        REQUIRE(a == dbl_approx(0));
      }
    }
    WHEN("concentration array of 1's") {
      DialogConcentrationImage dia(std::vector<double>(9, 1.0), specGeom);
      auto a = dia.getConcentrationArray();
      for (auto i : compArrayIndices) {
        REQUIRE(a[i] == dbl_approx(1.0));
      }
      for (auto i : outsideArrayIndices) {
        REQUIRE(a[i] == dbl_approx(0.0));
      }
    }
    WHEN("concentration array size mismatch") {
      REQUIRE_THROWS(
          DialogConcentrationImage(std::vector<double>(8, 0.0), specGeom));
      REQUIRE_THROWS(
          DialogConcentrationImage(std::vector<double>(10, 0.0), specGeom));
    }
    WHEN("concentration array of different values") {
      std::vector<double> arr{1.0, 3.0, 2.0, 4.0, 0.3, 0.1, 0.8, 0.5, 0.99};
      DialogConcentrationImage dia(arr, specGeom);
      auto a = dia.getConcentrationArray();
      for (auto i : compArrayIndices) {
        REQUIRE(a[i] == dbl_approx(arr[i]));
      }
      for (auto i : outsideArrayIndices) {
        REQUIRE(a[i] == dbl_approx(0.0));
      }
    }
    WHEN("min changed to valid value") {
      // initial range 0-1
      std::vector<double> arr{0.5, 0.0, 0.0, 0.25, 0.0, 0.0, 1.0, 0.0, 0.0};
      DialogConcentrationImage dia(arr, specGeom);
      // new range 0.5-1
      mwt.addUserAction({"Backspace", "Backspace", "Backspace", "Backspace",
                         "Backspace", "0", ".", "5", "Tab"});
      mwt.start();
      dia.exec();
      auto a = dia.getConcentrationArray();
      for (auto i : compArrayIndices) {
        REQUIRE(a[i] == dbl_approx(0.5 + 0.5 * arr[i]));
      }
      for (auto i : outsideArrayIndices) {
        REQUIRE(a[i] == dbl_approx(0.0));
      }
    }
    WHEN("max changed to valid value") {
      // initial range 0-1
      std::vector<double> arr{0.5, 0.0, 0.0, 0.25, 0.0, 0.0, 1.0, 0.0, 0.0};
      DialogConcentrationImage dia(arr, specGeom);
      // new range 0-5
      mwt.addUserAction({"Tab", "Backspace", "Backspace", "Backspace",
                         "Backspace", "5", "Tab"});
      mwt.start();
      dia.exec();
      auto a = dia.getConcentrationArray();
      for (auto i : compArrayIndices) {
        REQUIRE(a[i] == dbl_approx(5 * arr[i]));
      }
      for (auto i : outsideArrayIndices) {
        REQUIRE(a[i] == dbl_approx(0.0));
      }
    }
    WHEN("min & max changed to valid values") {
      // initial range 0-1
      std::vector<double> arr{0.5, 0.0, 0.0, 0.25, 0.0, 0.0, 1.0, 0.0, 0.0};
      DialogConcentrationImage dia(arr, specGeom);
      // new range 0.5-1.5
      mwt.addUserAction({"Backspace", "Backspace", "Backspace", "Backspace",
                         "0", ".", "5", "Tab", "Backspace", "Backspace",
                         "Backspace", "Backspace", "1", ".", "5", "Tab"});
      mwt.start();
      dia.exec();
      auto a = dia.getConcentrationArray();
      for (auto i : compArrayIndices) {
        REQUIRE(a[i] == dbl_approx(0.5 + arr[i]));
      }
      for (auto i : outsideArrayIndices) {
        REQUIRE(a[i] == dbl_approx(0.0));
      }
    }
    WHEN("max changed to min: invalid, set max -> min + 1") {
      // initial range 0-0.5
      std::vector<double> arr{0.5, 0.0, 0.0, 0.25, 0.0, 0.0, 0.111, 0.0, 0.0};
      DialogConcentrationImage dia(arr, specGeom);
      // new range 0-0 -> 0-1
      mwt.addUserAction({"Tab", "Backspace", "Backspace", "Backspace",
                         "Backspace", "0", "Tab"});
      mwt.start();
      dia.exec();
      auto a = dia.getConcentrationArray();
      for (auto i : compArrayIndices) {
        REQUIRE(a[i] == dbl_approx(2 * arr[i]));
      }
      for (auto i : outsideArrayIndices) {
        REQUIRE(a[i] == dbl_approx(0.0));
      }
    }
    WHEN("min negative: invalid, set min to zero") {
      // initial range 0.5-1
      std::vector<double> arr{0.5, 0.0, 0.0, 0.65, 0.0, 0.0, 1.0, 0.88, 0.0};
      DialogConcentrationImage dia(arr, specGeom);
      // new min = -2, invalid -> reset to 0: new range 0-1
      mwt.addUserAction(
          {"Backspace", "Backspace", "Backspace", "Backspace", "-", "2"});
      mwt.start();
      dia.exec();
      auto a = dia.getConcentrationArray();
      for (auto i : compArrayIndices) {
        REQUIRE(a[i] == dbl_approx(2.0 * arr[i] - 1.0));
      }
      for (auto i : outsideArrayIndices) {
        REQUIRE(a[i] == dbl_approx(0.0));
      }
    }
    WHEN("min set larger than max: invalid, set max -> min + 1") {
      // initial range 0.5-1
      std::vector<double> arr{0.5, 0.0, 0.0, 0.65, 0.0, 0.0, 1.0, 0.88, 0.0};
      DialogConcentrationImage dia(arr, specGeom);
      // new min = 2, invalid -> set max to 2+1: new range 2-3
      mwt.addUserAction({"Backspace", "Backspace", "Backspace", "2"});
      mwt.start();
      dia.exec();
      auto a = dia.getConcentrationArray();
      for (auto i : compArrayIndices) {
        REQUIRE(a[i] == dbl_approx(2.0 * arr[i] + 1.0));
      }
      for (auto i : outsideArrayIndices) {
        REQUIRE(a[i] == dbl_approx(0.0));
      }
    }
    WHEN("all-black pixel image loaded") {
      QImage concImg(3, 3, QImage::Format_ARGB32_Premultiplied);
      concImg.fill(QColor(0, 0, 0));
      concImg.save("tmp.png");
      DialogConcentrationImage dia({}, specGeom);
      // import image
      ModalWidgetTimer mwtImage;
      mwt.addUserAction({"Tab", "Tab", "Space"}, true, &mwtImage);
      mwtImage.addUserAction({"t", "m", "p", ".", "p", "n", "g"}, true);
      mwt.start();
      dia.exec();
      auto a = dia.getConcentrationArray();
      for (auto i : compArrayIndices) {
        REQUIRE(a[i] == dbl_approx(0.0));
      }
      for (auto i : outsideArrayIndices) {
        REQUIRE(a[i] == dbl_approx(0.0));
      }
    }
    WHEN("wrong size image loaded: rescaled") {
      QImage concImg(23, 64, QImage::Format_ARGB32_Premultiplied);
      concImg.fill(QColor(0, 0, 0));
      concImg.save("tmp.png");
      DialogConcentrationImage dia({}, specGeom);
      // import image
      ModalWidgetTimer mwtImage;
      mwt.addUserAction({"Tab", "Tab", "Space"}, true, &mwtImage);
      mwtImage.addUserAction({"t", "m", "p", ".", "p", "n", "g"});
      mwt.start();
      dia.exec();
      auto a = dia.getConcentrationArray();
      for (auto i : compArrayIndices) {
        REQUIRE(a[i] == dbl_approx(0.0));
      }
      for (auto i : outsideArrayIndices) {
        REQUIRE(a[i] == dbl_approx(0.0));
      }
    }
    WHEN("pixel image loaded") {
      QImage concImg(3, 3, QImage::Format_ARGB32_Premultiplied);
      concImg.fill(0);
      concImg.setPixel(0, 0, qRgb(0, 0, 0));
      concImg.setPixel(0, 1, qRgb(100, 100, 100));
      concImg.setPixel(0, 2, qRgb(50, 50, 50));
      concImg.setPixel(1, 0, qRgb(25, 25, 25));
      concImg.save("tmp.png");
      DialogConcentrationImage dia({}, specGeom);
      std::vector<double> values = {0, 1, 0.5, 0.25};
      // import image
      ModalWidgetTimer mwtImage;
      mwt.addUserAction({"Tab", "Tab", "Space"}, false, &mwtImage);
      mwtImage.addUserAction({"t", "m", "p", ".", "p", "n", "g"});
      THEN("get concentration array") {
        mwt.addUserAction();
        mwt.start();
        dia.exec();
        auto a = dia.getConcentrationArray();
        std::size_t iVal = 0;
        for (auto i : compArrayIndices) {
          REQUIRE(a[i] == dbl_approx(values[iVal++]));
        }
        for (auto i : outsideArrayIndices) {
          REQUIRE(a[i] == dbl_approx(0.0));
        }
      }
      THEN("export image") {
        // export image
        mwt.addUserAction({"Tab", "Space"}, false, &mwtImage);
        mwtImage.addUserAction({"q", "q", "q"});
        mwt.addUserAction();
        mwt.start();
        dia.exec();
        auto exportedImage = QImage("qqq.png");
        REQUIRE(exportedImage.size() == concImg.size());
        // exported image normalised to black=min, white=max:
        REQUIRE(exportedImage.pixel(0, 0) == qRgb(0, 0, 0));
        REQUIRE(exportedImage.pixel(0, 1) == qRgb(255, 255, 255));
        REQUIRE(exportedImage.pixel(0, 2) == qRgb(127, 127, 127));
        REQUIRE(exportedImage.pixel(1, 0) == qRgb(63, 63, 63));
      }
      THEN("smooth image") {
        // click on smooth image
        mwt.addUserAction({"Tab", "Tab", "Tab", "Space"});
        mwt.start();
        dia.exec();
        auto a = dia.getConcentrationArray();
        // points inside array are smoothed
        REQUIRE(a[6] == Catch::Approx(0.0605124));
        REQUIRE(a[3] == Catch::Approx(0.7482450));
        REQUIRE(a[0] == Catch::Approx(0.4349422));
        REQUIRE(a[7] == Catch::Approx(0.1930087));
        // points outside array should remain zero
        for (auto i : outsideArrayIndices) {
          REQUIRE(a[i] == dbl_approx(0.0));
        }
      }
    }
    WHEN("built-in image loaded") {
      DialogConcentrationImage dia({}, specGeom);
      // load built-in two-blobs image
      ModalWidgetTimer mwtImage;
      mwt.addUserAction(
          {"Tab", "Tab", "Tab", "Tab", "Space", "Down", "Down", "Enter"});
      mwt.start();
      dia.exec();
      auto a = dia.getConcentrationArray();
      REQUIRE(a[6] == Catch::Approx(1.0));
      REQUIRE(a[3] == Catch::Approx(0.0));
      REQUIRE(a[0] == Catch::Approx(0.0));
      REQUIRE(a[7] == Catch::Approx(1.0 / 11.0));
      for (auto i : outsideArrayIndices) {
        REQUIRE(a[i] == dbl_approx(0.0));
      }
    }
  }
}

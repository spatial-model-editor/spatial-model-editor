#include "catch_wrapper.hpp"
#include "dialogconcentrationimage.hpp"
#include "model_test_utils.hpp"
#include "qt_test_utils.hpp"
#include "sme/model.hpp"
#include "sme/model_units.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>
#include <numeric>

using namespace sme::test;

struct DialogConcentrationImageWidgets {
  explicit DialogConcentrationImageWidgets(
      const DialogConcentrationImage *dialog) {
    GET_DIALOG_WIDGET(QLineEdit, txtMinConc);
    GET_DIALOG_WIDGET(QLineEdit, txtMaxConc);
    GET_DIALOG_WIDGET(QCheckBox, chkGrid);
    GET_DIALOG_WIDGET(QCheckBox, chkScale);
    GET_DIALOG_WIDGET(QPushButton, btnImportImage);
    GET_DIALOG_WIDGET(QPushButton, btnExportImage);
    GET_DIALOG_WIDGET(QComboBox, cmbExampleImages);
    GET_DIALOG_WIDGET(QPushButton, btnSmoothImage);
    GET_DIALOG_WIDGET(QSlider, slideZIndex);
  }
  QLineEdit *txtMinConc;
  QLineEdit *txtMaxConc;
  QCheckBox *chkGrid;
  QCheckBox *chkScale;
  QPushButton *btnImportImage;
  QPushButton *btnExportImage;
  QComboBox *cmbExampleImages;
  QPushButton *btnSmoothImage;
  QSlider *slideZIndex;
};

TEST_CASE("DialogConcentrationImage", "[gui/dialogs/concentrationimage][gui/"
                                      "dialogs][gui][concentrationimage]") {
  auto compartmentVoxels = std::vector<sme::common::Voxel>{
      {0, 0, 0}, {0, 1, 0}, {0, 2, 0}, {1, 0, 0}};
  auto compArrayIndices = std::vector<std::size_t>{6, 3, 0, 7};
  auto outsideArrayIndices = std::vector<std::size_t>{1, 2, 4, 5, 8};
  sme::model::ModelUnits units{};
  sme::model::SpeciesGeometry specGeom{
      {3, 3, 1}, compartmentVoxels, {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, units};
  SECTION("empty concentration array: set concentration to 0") {
    DialogConcentrationImage dia({}, specGeom);
    for (const auto &a : dia.getConcentrationArray()) {
      REQUIRE(a == dbl_approx(0));
    }
  }
  SECTION("concentration array of 0's") {
    DialogConcentrationImage dia(std::vector<double>(9, 0.0), specGeom);
    for (const auto &a : dia.getConcentrationArray()) {
      REQUIRE(a == dbl_approx(0));
    }
  }
  SECTION("rate of change of concentration array of 0's with custom title") {
    DialogConcentrationImage dia(std::vector<double>(9, 0.0), specGeom, false,
                                 "Custom window title", true);
    for (const auto &a : dia.getConcentrationArray()) {
      REQUIRE(a == dbl_approx(0));
    }
    REQUIRE(dia.windowTitle() == "Custom window title");
  }
  SECTION("concentration array of 1's") {
    DialogConcentrationImage dia(std::vector<double>(9, 1.0), specGeom);
    auto a = dia.getConcentrationArray();
    for (auto i : compArrayIndices) {
      REQUIRE(a[i] == dbl_approx(1.0));
    }
    for (auto i : outsideArrayIndices) {
      REQUIRE(a[i] == dbl_approx(0.0));
    }
  }
  SECTION("concentration array size mismatch") {
    REQUIRE_THROWS(
        DialogConcentrationImage(std::vector<double>(8, 0.0), specGeom));
    REQUIRE_THROWS(
        DialogConcentrationImage(std::vector<double>(10, 0.0), specGeom));
  }
  SECTION("concentration array of different values") {
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
  SECTION("min changed to valid value") {
    // initial range 0-1
    std::vector<double> arr{0.5, 0.0, 0.0, 0.25, 0.0, 0.0, 1.0, 0.0, 0.0};
    DialogConcentrationImage dia(arr, specGeom);
    dia.show();
    DialogConcentrationImageWidgets widgets(&dia);
    // new range 0.5-1
    sendKeyEvents(widgets.txtMinConc,
                  {"Backspace", "Backspace", "Backspace", "Backspace",
                   "Backspace", "0", ".", "5", "Enter"});
    auto a = dia.getConcentrationArray();
    for (auto i : compArrayIndices) {
      REQUIRE(a[i] == dbl_approx(0.5 + 0.5 * arr[i]));
    }
    for (auto i : outsideArrayIndices) {
      REQUIRE(a[i] == dbl_approx(0.0));
    }
  }
  SECTION("max changed to valid value") {
    // initial range 0-1
    std::vector<double> arr{0.5, 0.0, 0.0, 0.25, 0.0, 0.0, 1.0, 0.0, 0.0};
    DialogConcentrationImage dia(arr, specGeom);
    dia.show();
    DialogConcentrationImageWidgets widgets(&dia);
    // new range 0-5
    sendKeyEvents(widgets.txtMaxConc, {"Backspace", "Backspace", "Backspace",
                                       "Backspace", "5", "Enter"});
    auto a = dia.getConcentrationArray();
    for (auto i : compArrayIndices) {
      REQUIRE(a[i] == dbl_approx(5 * arr[i]));
    }
    for (auto i : outsideArrayIndices) {
      REQUIRE(a[i] == dbl_approx(0.0));
    }
  }
  SECTION("min & max changed to valid values") {
    // initial range 0-1
    std::vector<double> arr{0.5, 0.0, 0.0, 0.25, 0.0, 0.0, 1.0, 0.0, 0.0};
    DialogConcentrationImage dia(arr, specGeom);
    dia.show();
    DialogConcentrationImageWidgets widgets(&dia);
    // new range 0.5-1.5
    sendKeyEvents(widgets.txtMinConc, {"Backspace", "Backspace", "Backspace",
                                       "Backspace", "0", ".", "5", "Enter"});
    sendKeyEvents(widgets.txtMaxConc, {"Backspace", "Backspace", "Backspace",
                                       "Backspace", "1", ".", "5", "Enter"});
    auto a = dia.getConcentrationArray();
    for (auto i : compArrayIndices) {
      REQUIRE(a[i] == dbl_approx(0.5 + arr[i]));
    }
    for (auto i : outsideArrayIndices) {
      REQUIRE(a[i] == dbl_approx(0.0));
    }
  }
  SECTION("max changed to min: invalid, set max -> min + 1") {
    // initial range 0-0.5
    std::vector<double> arr{0.5, 0.0, 0.0, 0.25, 0.0, 0.0, 0.111, 0.0, 0.0};
    DialogConcentrationImage dia(arr, specGeom);
    dia.show();
    DialogConcentrationImageWidgets widgets(&dia);
    // new range 0-0 -> invalid, resets to 0-1
    sendKeyEvents(widgets.txtMaxConc, {"Backspace", "Backspace", "Backspace",
                                       "Backspace", "0", "Enter"});
    auto a = dia.getConcentrationArray();
    for (auto i : compArrayIndices) {
      REQUIRE(a[i] == dbl_approx(2 * arr[i]));
    }
    for (auto i : outsideArrayIndices) {
      REQUIRE(a[i] == dbl_approx(0.0));
    }
  }
  SECTION("min negative: invalid, set min to zero") {
    // initial range 0.5-1
    std::vector<double> arr{0.5, 0.0, 0.0, 0.65, 0.0, 0.0, 1.0, 0.88, 0.0};
    DialogConcentrationImage dia(arr, specGeom);
    dia.show();
    DialogConcentrationImageWidgets widgets(&dia);
    // new min = -2, invalid -> reset to 0: new range 0-1
    sendKeyEvents(widgets.txtMinConc, {"Backspace", "Backspace", "Backspace",
                                       "Backspace", "-", "2", "Enter"});
    auto a = dia.getConcentrationArray();
    for (auto i : compArrayIndices) {
      REQUIRE(a[i] == dbl_approx(2.0 * arr[i] - 1.0));
    }
    for (auto i : outsideArrayIndices) {
      REQUIRE(a[i] == dbl_approx(0.0));
    }
  }
  SECTION("min set larger than max: invalid, set max -> min + 1") {
    // initial range 0.5-1
    std::vector<double> arr{0.5, 0.0, 0.0, 0.65, 0.0, 0.0, 1.0, 0.88, 0.0};
    DialogConcentrationImage dia(arr, specGeom);
    dia.show();
    DialogConcentrationImageWidgets widgets(&dia);
    // new min = 2, invalid -> set max to 2+1: new range 2-3
    sendKeyEvents(widgets.txtMinConc,
                  {"Backspace", "Backspace", "Backspace", "2", "Enter"});
    auto a = dia.getConcentrationArray();
    for (auto i : compArrayIndices) {
      REQUIRE(a[i] == dbl_approx(2.0 * arr[i] + 1.0));
    }
    for (auto i : outsideArrayIndices) {
      REQUIRE(a[i] == dbl_approx(0.0));
    }
  }
  SECTION("all-black pixel image loaded") {
    QImage concImg(3, 3, QImage::Format_ARGB32_Premultiplied);
    concImg.fill(QColor(0, 0, 0));
    concImg.save("tmpdci.png");
    // import image
    DialogConcentrationImage dia({}, specGeom);
    dia.show();
    DialogConcentrationImageWidgets widgets(&dia);
    ModalWidgetTimer mwt;
    mwt.addUserAction({"t", "m", "p", "d", "c", "i", ".", "p", "n", "g"}, true);
    mwt.start();
    sendMouseClick(widgets.btnImportImage);
    auto a = dia.getConcentrationArray();
    for (auto i : compArrayIndices) {
      REQUIRE(a[i] == dbl_approx(0.0));
    }
    for (auto i : outsideArrayIndices) {
      REQUIRE(a[i] == dbl_approx(0.0));
    }
  }
  SECTION("wrong size image loaded: rescaled") {
    QImage concImg(23, 64, QImage::Format_ARGB32_Premultiplied);
    concImg.fill(QColor(0, 0, 0));
    concImg.save("tmpdci.png");
    DialogConcentrationImage dia({}, specGeom);
    dia.show();
    DialogConcentrationImageWidgets widgets(&dia);
    // import image
    ModalWidgetTimer mwt;
    mwt.addUserAction({"t", "m", "p", "d", "c", "i", ".", "p", "n", "g"});
    mwt.start();
    sendMouseClick(widgets.btnImportImage);
    auto a = dia.getConcentrationArray();
    for (auto i : compArrayIndices) {
      REQUIRE(a[i] == dbl_approx(0.0));
    }
    for (auto i : outsideArrayIndices) {
      REQUIRE(a[i] == dbl_approx(0.0));
    }
  }
  SECTION("pixel image loaded") {
    QImage concImg(3, 3, QImage::Format_ARGB32_Premultiplied);
    concImg.fill(0);
    concImg.setPixel(0, 0, qRgb(0, 0, 0));
    concImg.setPixel(0, 1, qRgb(100, 100, 100));
    concImg.setPixel(0, 2, qRgb(50, 50, 50));
    concImg.setPixel(1, 0, qRgb(25, 25, 25));
    concImg.save("tmpdci.png");
    DialogConcentrationImage dia({}, specGeom);
    dia.show();
    DialogConcentrationImageWidgets widgets(&dia);
    std::vector<double> values = {0, 1, 0.5, 0.25};
    // import image
    ModalWidgetTimer mwtImageImport;
    mwtImageImport.addUserAction(
        {"t", "m", "p", "d", "c", "i", ".", "p", "n", "g"});
    mwtImageImport.start();
    sendMouseClick(widgets.btnImportImage);
    SECTION("get concentration array") {
      auto a = dia.getConcentrationArray();
      std::size_t iVal = 0;
      for (auto i : compArrayIndices) {
        REQUIRE(a[i] == dbl_approx(values[iVal++]));
      }
      for (auto i : outsideArrayIndices) {
        REQUIRE(a[i] == dbl_approx(0.0));
      }
    }
    SECTION("export image") {
      // export image
      ModalWidgetTimer mwtImageExport;
      mwtImageExport.addUserAction({"t", "m", "p", "d", "c", "i", "o"});
      mwtImageExport.start();
      sendMouseClick(widgets.btnExportImage);
      QImage exportedImage("tmpdcio.png");
      REQUIRE(exportedImage.size() == concImg.size());
      // exported image normalised to black=min, white=max:
      REQUIRE(exportedImage.pixel(0, 0) == qRgb(0, 0, 0));
      REQUIRE(exportedImage.pixel(0, 1) == qRgb(255, 255, 255));
      REQUIRE(exportedImage.pixel(0, 2) == qRgb(127, 127, 127));
      REQUIRE(exportedImage.pixel(1, 0) == qRgb(63, 63, 63));
    }
    SECTION("smooth image") {
      sendMouseClick(widgets.btnSmoothImage);
      auto a = dia.getConcentrationArray();
      // points inside array are smoothed
      REQUIRE(a[6] > 0.0);
      REQUIRE(a[3] < 1.0);
      REQUIRE(a[0] < 0.5);
      REQUIRE(a[7] < 0.25);
      // points outside array should remain zero
      for (auto i : outsideArrayIndices) {
        REQUIRE(a[i] == dbl_approx(0.0));
      }
    }
  }
  SECTION("built-in image loaded") {
    DialogConcentrationImage dia({}, specGeom);
    dia.show();
    DialogConcentrationImageWidgets widgets(&dia);
    sendMouseClick(widgets.chkGrid);
    sendMouseClick(widgets.chkScale);
    // load built-in two-blobs image
    widgets.cmbExampleImages->setCurrentIndex(2);
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

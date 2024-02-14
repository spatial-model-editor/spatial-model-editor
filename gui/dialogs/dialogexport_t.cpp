#include "catch_wrapper.hpp"
#include "dialogexport.hpp"
#include "model_test_utils.hpp"
#include "plotwrapper.hpp"
#include "qt_test_utils.hpp"
#include "sme/model.hpp"
#include "sme/serialization.hpp"
#include "sme/simulate.hpp"
#include "sme/simulate_options.hpp"
#include <QComboBox>
#include <QRadioButton>

using namespace sme::test;

struct DialogExportWidgets {
  explicit DialogExportWidgets(const DialogExport *dialog) {
    GET_DIALOG_WIDGET(QRadioButton, radAllImages);
    GET_DIALOG_WIDGET(QRadioButton, radCSV);
    GET_DIALOG_WIDGET(QComboBox, cmbTimepoint);
    GET_DIALOG_WIDGET(QRadioButton, radModel);
    GET_DIALOG_WIDGET(QRadioButton, radSingleImage);
  }
  QRadioButton *radAllImages;
  QRadioButton *radCSV;
  QComboBox *cmbTimepoint;
  QRadioButton *radModel;
  QRadioButton *radSingleImage;
};

TEST_CASE("DialogExport", "[gui/dialogs/export][gui/dialogs][gui][export]") {
  auto model{getExampleModel(Mod::VerySimpleModel)};
  model.getSimulationSettings().simulatorType =
      sme::simulate::SimulatorType::Pixel;
  sme::simulate::Simulation sim(model);
  sme::simulate::Options options;
  sim.doTimesteps(0.001, 4);
  sme::common::ImageStack imgGeometry({100, 50, 1},
                                      QImage::Format_ARGB32_Premultiplied);
  QVector<sme::common::ImageStack> imgs(5, imgGeometry);
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
  QWidget plotParent;
  PlotWrapper plot("title", &plotParent);
  plot.addAvMinMaxLine("l1", QColor(12, 12, 12));
  plot.addAvMinMaxPoint(0, 0.0, {1.0, 0.99, 1.11});
  plot.addAvMinMaxPoint(0, 1.0, {1.0, 1.0, 1.0});
  plot.addAvMinMaxPoint(0, 2.0, {1.0, 1.0, 1.0});
  plot.addAvMinMaxPoint(0, 3.0, {1.0, 1.0, 1.0});
  plot.addAvMinMaxPoint(0, 4.0, {1.0, 1.0, 1.0});
  DialogExport dia(imgs, &plot, model, sim, 2);
  dia.show();
  DialogExportWidgets widgets(&dia);
  SECTION("user clicks export to model") {
    std::size_t i{18};
    double conc0{model.getSpecies().getSampledFieldConcentration("A_c2")[i]};
    REQUIRE(conc0 == dbl_approx(0.0));
    // change initial concentration in model
    model.getSpecies().setInitialConcentration("A_c2", 0.123);
    REQUIRE(model.getSpecies().getSampledFieldConcentration("A_c2")[i] ==
            dbl_approx(0.123));
    // export t=0 simulation concs to model
    widgets.cmbTimepoint->setCurrentIndex(0);
    sendMouseClick(widgets.radModel);
    sendKeyEvents(&dia, {"Enter"});
    REQUIRE(model.getSpecies().getSampledFieldConcentration("A_c2")[i] ==
            dbl_approx(conc0));
  }
  SECTION("user clicks save image, then cancel") {
    ModalWidgetTimer mwt;
    mwt.addUserAction({"Esc"});
    mwt.start();
    sendMouseClick(widgets.radSingleImage);
    sendKeyEvents(&dia, {"Enter"});
    REQUIRE(mwt.getResult() == "QFileDialog::AcceptSave");
  }
  SECTION("user clicks save image, then enters filename") {
    ModalWidgetTimer mwt;
    mwt.addUserAction({"t", "m", "p", "d", "i", "a", "e", "x"});
    mwt.start();
    sendMouseClick(widgets.radSingleImage);
    sendKeyEvents(&dia, {"Enter"});
    REQUIRE(mwt.getResult() == "QFileDialog::AcceptSave");
    QImage img("tmpdiaex.png");
    REQUIRE(img.size().width() == imgs[0].volume().width());
    REQUIRE(img.size().height() == imgs[0].volume().height());
    REQUIRE(img.pixel(0, 0) == col2);
  }
  SECTION("user changes timepoint, clicks save image, then enters filename") {
    ModalWidgetTimer mwt;
    mwt.addUserAction({"t", "m", "p", "d", "i", "a", "e", "x"});
    mwt.start();
    widgets.cmbTimepoint->setCurrentIndex(4);
    sendMouseClick(widgets.radSingleImage);
    sendKeyEvents(&dia, {"Enter"});
    REQUIRE(mwt.getResult() == "QFileDialog::AcceptSave");
    QImage img("tmpdiaex.png");
    REQUIRE(img.size().width() == imgs[0].volume().width());
    REQUIRE(img.size().height() == imgs[0].volume().height());
    REQUIRE(img.pixel(0, 0) == col4);
  }
  SECTION("user clicks on All timepoints, clicks save image, then enter") {
    ModalWidgetTimer mwt;
    mwt.addUserAction({"Enter"});
    mwt.start();
    sendMouseClick(widgets.radAllImages);
    sendKeyEvents(&dia, {"Enter"});
    REQUIRE(mwt.getResult() == "QFileDialog::AcceptOpen");
    QImage img0("img0.png");
    REQUIRE(img0.size().width() == imgs[0].volume().width());
    REQUIRE(img0.size().height() == imgs[0].volume().height());
    REQUIRE(img0.pixel(0, 0) == col0);
    QImage img1("img1.png");
    REQUIRE(img1.size().width() == imgs[1].volume().width());
    REQUIRE(img1.size().height() == imgs[1].volume().height());
    REQUIRE(img1.pixel(0, 0) == col1);
    QImage img2("img2.png");
    REQUIRE(img2.size().width() == imgs[2].volume().width());
    REQUIRE(img2.size().height() == imgs[2].volume().height());
    REQUIRE(img2.pixel(0, 0) == col2);
    QImage img3("img3.png");
    REQUIRE(img3.size().width() == imgs[3].volume().width());
    REQUIRE(img3.size().height() == imgs[3].volume().height());
    REQUIRE(img3.pixel(0, 0) == col3);
    QImage img4("img4.png");
    REQUIRE(img4.size().width() == imgs[4].volume().width());
    REQUIRE(img4.size().height() == imgs[4].volume().height());
    REQUIRE(img4.pixel(0, 0) == col4);
  }
  SECTION("user clicks on csv, types filename, then enter") {
    ModalWidgetTimer mwt;
    mwt.addUserAction({"t", "m", "p", "d", "i", "a", "e", "x"});
    mwt.start();
    sendMouseClick(widgets.radCSV);
    sendKeyEvents(&dia, {"Enter"});
    REQUIRE(mwt.getResult() == "QFileDialog::AcceptSave");
    QFile f2("tmpdiaex.txt");
    f2.open(QIODevice::ReadOnly | QIODevice::Text);
    QTextStream in(&f2);
    QStringList lines;
    while (!in.atEnd()) {
      lines.push_back(in.readLine());
    }
    REQUIRE(lines.size() == 6);
    REQUIRE(lines[0] == "time, l1 (avg), l1 (min), l1 (max)");
    REQUIRE(lines[1] == "0.00000000000000e+00, 1.00000000000000e+00, "
                        "9.90000000000000e-01, 1.11000000000000e+00");
  }
}

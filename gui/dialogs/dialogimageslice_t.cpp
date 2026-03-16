#include "catch_wrapper.hpp"
#include "dialogimageslice.hpp"
#include "model_test_utils.hpp"
#include "qlabelmousetracker.hpp"
#include "qlabelslice.hpp"
#include "qt_test_utils.hpp"
#include "sme/simulate.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QFile>
#include <QLabel>
#include <QSlider>
#include <QSplitter>
#include <algorithm>
#include <cmath>
#include <qcustomplot.h>

using namespace sme::test;

struct DialogImageSliceWidgets {
  explicit DialogImageSliceWidgets(const DialogImageSlice *dialog) {
    GET_DIALOG_WIDGET(QSplitter, splitter);
    GET_DIALOG_WIDGET(QComboBox, cmbSliceType);
    GET_DIALOG_WIDGET(QCheckBox, chkAspectRatio);
    GET_DIALOG_WIDGET(QCheckBox, chkSmoothInterpolation);
    GET_DIALOG_WIDGET(QCheckBox, chkAutoscaleYAxis);
    GET_DIALOG_WIDGET(QLabelSlice, lblSlice);
    GET_DIALOG_WIDGET(QLabelMouseTracker, lblImage);
    GET_DIALOG_WIDGET(QLabel, lblMouseLocation);
    GET_DIALOG_WIDGET(QLabel, lblSelectedTimepoint);
    GET_DIALOG_WIDGET(QSlider, slideTimepoint);
    GET_DIALOG_WIDGET(QCustomPlot, concentrationPlot);
  }
  QSplitter *splitter;
  QComboBox *cmbSliceType;
  QCheckBox *chkAspectRatio;
  QCheckBox *chkSmoothInterpolation;
  QCheckBox *chkAutoscaleYAxis;
  QLabelSlice *lblSlice;
  QLabelMouseTracker *lblImage;
  QLabel *lblMouseLocation;
  QLabel *lblSelectedTimepoint;
  QSlider *slideTimepoint;
  QCustomPlot *concentrationPlot;
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
  auto splitterSizes{widgets.splitter->sizes()};
  REQUIRE(splitterSizes.size() == 2);
  REQUIRE(splitterSizes[1] > splitterSizes[0]);
  REQUIRE(splitterSizes[1] >= 1.7 * splitterSizes[0]);
  REQUIRE(splitterSizes[1] <= 2.3 * splitterSizes[0]);
  REQUIRE(widgets.chkSmoothInterpolation->isChecked() == false);
  REQUIRE(widgets.chkAutoscaleYAxis->isChecked() == false);
  REQUIRE(widgets.chkAutoscaleYAxis->text() == "Autoscale y-axis");
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

  SECTION("simulation-backed plot updates with timepoint slider") {
    auto model{getExampleModel(Mod::ABtoC)};
    model.getSimulationSettings().times.clear();
    sme::simulate::Simulation sim(model);
    sim.doMultipleTimesteps({{2, 0.01}});

    QVector<double> timepoints;
    for (double t : sim.getTimePoints()) {
      timepoints.push_back(t);
    }
    QVector<sme::common::ImageStack> concImages;
    concImages.reserve(static_cast<int>(timepoints.size()));
    for (std::size_t i = 0; i < static_cast<std::size_t>(timepoints.size());
         ++i) {
      concImages.push_back(sim.getConcImage(i));
    }
    DialogImageSlicePlotData plotData;
    plotData.simulation = &sim;
    plotData.speciesToDraw = {{0}};
    for (const auto &compartmentId : sim.getCompartmentIds()) {
      plotData.compartmentNames.push_back(
          model.getCompartments().getName(compartmentId.c_str()));
    }
    plotData.timeUnit = model.getUnits().getTime().name;
    plotData.lengthUnit = model.getUnits().getLength().name;
    plotData.concentrationUnit = model.getUnits().getConcentration();
    plotData.timepointIndex = 1;

    DialogImageSlice plottedDialog(model.getGeometry().getImages(), concImages,
                                   timepoints, false, plotData);
    plottedDialog.show();
    DialogImageSliceWidgets plottedWidgets(&plottedDialog);

    REQUIRE(plottedWidgets.slideTimepoint->maximum() ==
            static_cast<int>(timepoints.size()) - 1);
    REQUIRE(plottedWidgets.slideTimepoint->value() == 1);
    REQUIRE(plottedWidgets.lblSelectedTimepoint->text().contains("0.01"));
    REQUIRE(plottedWidgets.concentrationPlot->graphCount() == 1);
    REQUIRE(plottedWidgets.concentrationPlot->graph(0)->dataCount() ==
            model.getGeometry().getImages().volume().height());
    REQUIRE(plottedWidgets.chkSmoothInterpolation->isChecked() == false);
    REQUIRE(plottedWidgets.chkAutoscaleYAxis->isChecked() == false);
    REQUIRE(plottedWidgets.lblImage->getImage()[0] ==
            plottedDialog.getSlicedImage());

    auto getSliceLength = [&]() {
      const auto &pixels{plottedWidgets.lblSlice->getSlicePixels()};
      const auto &voxelSize{model.getGeometry().getImages().voxelSize()};
      double distance{0.0};
      for (std::size_t i = 1; i < pixels.size(); ++i) {
        const auto dx{static_cast<double>(pixels[i].x() - pixels[i - 1].x()) *
                      voxelSize.width()};
        const auto dy{static_cast<double>(pixels[i].y() - pixels[i - 1].y()) *
                      voxelSize.height()};
        distance += std::hypot(dx, dy);
      }
      return distance;
    };
    auto getConcentrationRange = [&](int firstTimepoint, int lastTimepoint) {
      const auto &pixels{plottedWidgets.lblSlice->getSlicePixels()};
      const auto &volume{model.getGeometry().getImages().volume()};
      bool haveRange{false};
      double minValue{0.0};
      double maxValue{0.0};
      for (int it = firstTimepoint; it < lastTimepoint; ++it) {
        const auto concArray{
            sim.getConcArray(static_cast<std::size_t>(it), 0, 0)};
        for (const auto &pixel : pixels) {
          const auto arrayIndex{
              sme::common::voxelArrayIndex(volume, pixel.x(), pixel.y(), 0)};
          const double value{concArray[arrayIndex]};
          if (!haveRange) {
            minValue = value;
            maxValue = value;
            haveRange = true;
            continue;
          }
          minValue = std::min(minValue, value);
          maxValue = std::max(maxValue, value);
        }
      }
      return std::pair(minValue, maxValue);
    };

    const auto [expectedMinConcentration, expectedMaxConcentration] =
        getConcentrationRange(0, static_cast<int>(timepoints.size()));
    const double expectedSliceLength{getSliceLength()};

    auto fixedXAxisRange{plottedWidgets.concentrationPlot->xAxis->range()};
    auto fixedYAxisRange{plottedWidgets.concentrationPlot->yAxis->range()};
    REQUIRE(fixedXAxisRange.lower == dbl_approx(0.0));
    REQUIRE(fixedXAxisRange.upper == dbl_approx(expectedSliceLength));
    REQUIRE(fixedYAxisRange.lower == dbl_approx(expectedMinConcentration));
    REQUIRE(fixedYAxisRange.upper == dbl_approx(expectedMaxConcentration));

    plottedWidgets.slideTimepoint->setValue(2);
    REQUIRE(plottedWidgets.lblSelectedTimepoint->text().contains("0.02"));
    REQUIRE(plottedWidgets.lblImage->getImage()[0] ==
            plottedDialog.getSlicedImage());
    auto fixedXAxisRangeAtLaterTime{
        plottedWidgets.concentrationPlot->xAxis->range()};
    auto fixedYAxisRangeAtLaterTime{
        plottedWidgets.concentrationPlot->yAxis->range()};
    REQUIRE(fixedXAxisRangeAtLaterTime.lower ==
            dbl_approx(fixedXAxisRange.lower));
    REQUIRE(fixedXAxisRangeAtLaterTime.upper ==
            dbl_approx(fixedXAxisRange.upper));
    REQUIRE(fixedYAxisRangeAtLaterTime.lower ==
            dbl_approx(fixedYAxisRange.lower));
    REQUIRE(fixedYAxisRangeAtLaterTime.upper ==
            dbl_approx(fixedYAxisRange.upper));

    plottedWidgets.chkAutoscaleYAxis->setChecked(true);
    const auto [expectedAutoscaledMinConcentration,
                expectedAutoscaledMaxConcentration] =
        getConcentrationRange(2, 3);
    auto autoscaledXAxisRange{plottedWidgets.concentrationPlot->xAxis->range()};
    auto autoscaledYAxisRange{plottedWidgets.concentrationPlot->yAxis->range()};
    REQUIRE(autoscaledXAxisRange.lower == dbl_approx(0.0));
    REQUIRE(autoscaledXAxisRange.upper == dbl_approx(expectedSliceLength));
    REQUIRE(autoscaledYAxisRange.lower ==
            dbl_approx(expectedAutoscaledMinConcentration));
    REQUIRE(autoscaledYAxisRange.upper ==
            dbl_approx(expectedAutoscaledMaxConcentration));
    REQUIRE(autoscaledYAxisRange.upper < fixedYAxisRange.upper);
  }
}

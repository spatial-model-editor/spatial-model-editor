#include "dialogimageslice.hpp"
#include "sme/logger.hpp"
#include "sme/model_geometry.hpp"
#include "sme/simulate.hpp"
#include "ui_dialogimageslice.h"
#include <QFileDialog>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <qcustomplot.h>
#include <utility>

namespace {

void setAxisRange(QCPAxis *axis, double minValue, double maxValue) {
  if (!std::isfinite(minValue) || !std::isfinite(maxValue)) {
    axis->setRange(0.0, 1.0);
    return;
  }
  if (minValue > maxValue) {
    std::swap(minValue, maxValue);
  }
  if (minValue == maxValue) {
    const double padding{minValue == 0.0 ? 1.0 : 0.05 * std::abs(minValue)};
    axis->setRange(minValue - padding, maxValue + padding);
    return;
  }
  axis->setRange(minValue, maxValue);
}

QString toQStr(double x) { return QString::number(x, 'g', 14); }

QString formatValueWithUnit(double value, const QString &unit) {
  if (unit.isEmpty()) {
    return toQStr(value);
  }
  return QString("%1 %2").arg(toQStr(value), unit);
}

} // namespace

DialogImageSlice::DialogImageSlice(
    const sme::common::ImageStack &geometryImage,
    const QVector<sme::common::ImageStack> &images,
    const QVector<double> &timepoints, bool invertYAxis,
    DialogImageSlicePlotData plotData, QWidget *parent)
    : QDialog(parent), ui{std::make_unique<Ui::DialogImageSlice>()},
      imgs{images}, time{timepoints}, plotData{std::move(plotData)},
      startPoint{0, geometryImage.volume().height() - 1},
      endPoint{geometryImage.volume().width() - 1, 0} {
  ui->setupUi(this);

  if (!geometryImage.empty()) {
    zIndex = static_cast<std::size_t>(
        std::clamp(plotData.zIndex, 0,
                   static_cast<int>(geometryImage.volume().depth()) - 1));
  }

  ui->lblImage->setAspectRatioMode(Qt::IgnoreAspectRatio);
  ui->lblImage->setTransformationMode(Qt::FastTransformation);
  ui->lblSlice->setImage(geometryImage[zIndex], invertYAxis);
  ui->lblSlice->setPhysicalUnits(plotData.lengthUnit);
  if (plotData.geometry != nullptr) {
    ui->lblSlice->setPhysicalOrigin(plotData.geometry->getPhysicalOrigin());
  }
  ui->lblSlice->setPhysicalSize(
      {geometryImage.voxelSize().width() *
           static_cast<double>(geometryImage.volume().width()),
       geometryImage.voxelSize().height() *
           static_cast<double>(geometryImage.volume().height()),
       1.0});
  initConcentrationPlot();

  connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
          &DialogImageSlice::saveSlicedImage);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
          &DialogImageSlice::reject);
  connect(ui->cmbSliceType, qOverload<int>(&QComboBox::activated), this,
          &DialogImageSlice::cmbSliceType_activated);
  connect(ui->chkAspectRatio, &QCheckBox::clicked, this,
          [lbl = ui->lblImage](bool checked) {
            if (checked) {
              lbl->setAspectRatioMode(Qt::KeepAspectRatio);
            } else {
              lbl->setAspectRatioMode(Qt::IgnoreAspectRatio);
            }
          });
  connect(ui->chkSmoothInterpolation, &QCheckBox::clicked, this,
          [lbl = ui->lblImage](bool checked) {
            if (checked) {
              lbl->setTransformationMode(Qt::SmoothTransformation);
            } else {
              lbl->setTransformationMode(Qt::FastTransformation);
            }
          });
  connect(ui->chkGrid, &QCheckBox::toggled, this,
          [lbl = ui->lblSlice](bool checked) { lbl->displayGrid(checked); });
  connect(ui->chkScale, &QCheckBox::toggled, this,
          [lbl = ui->lblSlice](bool checked) { lbl->displayScale(checked); });
  connect(ui->lblSlice, &QLabelSlice::mouseDown, this,
          &DialogImageSlice::lblSlice_mouseDown);
  connect(ui->lblSlice, &QLabelSlice::sliceDrawn, this,
          &DialogImageSlice::lblSlice_sliceDrawn);
  connect(ui->lblSlice, &QLabelSlice::mouseWheelEvent, this,
          &DialogImageSlice::lblSlice_mouseWheelEvent);
  connect(ui->lblSlice, &QLabelSlice::mouseOver, this,
          &DialogImageSlice::lblSlice_mouseOver);
  connect(ui->lblImage, &QLabelMouseTracker::mouseOver, this,
          &DialogImageSlice::lblImage_mouseOver);

  if (!time.isEmpty()) {
    selectedTimepointIndex = plotData.timepointIndex;
    if (selectedTimepointIndex < 0 ||
        selectedTimepointIndex >= static_cast<int>(time.size())) {
      selectedTimepointIndex = static_cast<int>(time.size()) - 1;
    }
    ui->slideTimepoint->setMaximum(static_cast<int>(time.size()) - 1);
    ui->slideTimepoint->setValue(selectedTimepointIndex);
  }
  updateSelectedTimepointText();

  // initial slice type: vertical
  ui->cmbSliceType->setCurrentIndex(1);
  cmbSliceType_activated(ui->cmbSliceType->currentIndex());
}

DialogImageSlice::~DialogImageSlice() = default;

QImage DialogImageSlice::getSlicedImage() const { return slice[0]; }

void DialogImageSlice::initConcentrationPlot() {
  ui->gridLayout->setRowStretch(2, 1);
  ui->gridLayoutImagePane->setRowStretch(2, 1);
  ui->gridLayoutPlotPane->setRowStretch(3, 1);
  ui->splitter->setStretchFactor(0, 1);
  ui->splitter->setStretchFactor(1, 2);
  ui->splitter->setSizes({300, 600});
  ui->slideTimepoint->setMinimum(0);
  ui->slideTimepoint->setMaximum(0);
  ui->slideTimepoint->setEnabled(!time.isEmpty());
  ui->concentrationPlot->setInteraction(QCP::iRangeDrag, true);
  ui->concentrationPlot->setInteraction(QCP::iRangeZoom, true);
  ui->concentrationPlot->setInteraction(QCP::iSelectPlottables, false);
  ui->concentrationPlot->legend->setVisible(true);
  ui->splitterImagePlot->setStretchFactor(0, 1);
  ui->splitterImagePlot->setStretchFactor(1, 0);
  ui->splitterImagePlot->setSizes({360, 220});

  connect(ui->slideTimepoint, &QSlider::valueChanged, this,
          &DialogImageSlice::slideTimepoint_valueChanged);
  connect(ui->chkAutoscaleYAxis, &QCheckBox::toggled, this,
          [this](bool) { updateConcentrationPlot(); });
}

void DialogImageSlice::updateSlicePlotData() {
  const auto &pixels = ui->lblSlice->getSlicePixels();
  distanceAlongSlice.clear();
  sliceArrayIndices.clear();
  distanceAlongSlice.reserve(static_cast<int>(pixels.size()));
  sliceArrayIndices.reserve(pixels.size());
  if (pixels.empty()) {
    fixedAxisRangesValid = false;
    return;
  }
  const auto &voxelSize{imgs[0].voxelSize()};
  const auto &volume{imgs[0].volume()};
  double distance{0.0};
  distanceAlongSlice.push_back(0.0);
  sliceArrayIndices.push_back(sme::common::voxelArrayIndex(
      volume, pixels.front().x(), pixels.front().y(), zIndex));
  for (std::size_t i = 1; i < pixels.size(); ++i) {
    const auto dx{static_cast<double>(pixels[i].x() - pixels[i - 1].x()) *
                  voxelSize.width()};
    const auto dy{static_cast<double>(pixels[i].y() - pixels[i - 1].y()) *
                  voxelSize.height()};
    distance += std::hypot(dx, dy);
    distanceAlongSlice.push_back(distance);
    sliceArrayIndices.push_back(sme::common::voxelArrayIndex(
        volume, pixels[i].x(), pixels[i].y(), zIndex));
  }
  fixedAxisRangesValid = false;
}

void DialogImageSlice::updateFixedAxisRanges() {
  fixedAxisRangesValid = false;
  if (plotData.simulation == nullptr || distanceAlongSlice.isEmpty() ||
      sliceArrayIndices.empty() || time.isEmpty()) {
    return;
  }

  fixedXAxisMin = distanceAlongSlice.front();
  fixedXAxisMax = distanceAlongSlice.back();

  const auto &sim{*plotData.simulation};
  const bool showAllSpecies{plotData.speciesToDraw.empty()};
  bool haveYRange{false};
  for (std::size_t ic = 0; ic < sim.getCompartmentIds().size(); ++ic) {
    std::vector<std::size_t> speciesIndices;
    if (showAllSpecies) {
      speciesIndices.resize(sim.getSpeciesIds(ic).size());
      std::iota(speciesIndices.begin(), speciesIndices.end(), 0);
    } else if (ic < plotData.speciesToDraw.size()) {
      speciesIndices = plotData.speciesToDraw[ic];
    }
    for (std::size_t is : speciesIndices) {
      for (int it = 0; it < time.size(); ++it) {
        const auto concArray{
            sim.getConcArray(static_cast<std::size_t>(it), ic, is)};
        for (std::size_t arrayIndex : sliceArrayIndices) {
          const double concentration{concArray[arrayIndex]};
          if (!haveYRange) {
            fixedYAxisMin = concentration;
            fixedYAxisMax = concentration;
            haveYRange = true;
            continue;
          }
          fixedYAxisMin = std::min(fixedYAxisMin, concentration);
          fixedYAxisMax = std::max(fixedYAxisMax, concentration);
        }
      }
    }
  }
  if (!haveYRange) {
    fixedYAxisMin = 0.0;
    fixedYAxisMax = 1.0;
  }
  fixedAxisRangesValid = true;
}

void DialogImageSlice::updateSlicedImage() {
  const auto &pixels = ui->lblSlice->getSlicePixels();
  auto np{static_cast<int>(pixels.size())};
  auto nt{static_cast<int>(time.size())};
  slice =
      sme::common::ImageStack({nt, np, 1}, QImage::Format_ARGB32_Premultiplied);
  int t = 0;
  for (const auto &img : imgs) {
    int y = np - 1;
    for (const auto &pixel : pixels) {
      slice[0].setPixel(t, y, img[zIndex].pixel(pixel));
      --y;
    }
    ++t;
  }
  updateSlicePlotData();
  updateDisplayedSlicedImage();
  updateConcentrationPlot();
}

void DialogImageSlice::updateDisplayedSlicedImage() {
  ui->lblImage->setVerticalIndicatorSourceX(
      time.isEmpty() ? -1 : selectedTimepointIndex);
  ui->lblImage->setImage(slice);
}

void DialogImageSlice::updateSelectedTimepointText() {
  QString text;
  if (!time.isEmpty()) {
    text = QString("Displayed time: %1")
               .arg(formatValueWithUnit(time[selectedTimepointIndex],
                                        plotData.timeUnit));
  }
  ui->lblSelectedTimepoint->setText(text);
}

void DialogImageSlice::updateConcentrationPlot() {
  ui->concentrationPlot->clearGraphs();
  ui->concentrationPlot->xAxis->setLabel(
      plotData.lengthUnit.isEmpty()
          ? "distance along slice"
          : QString("distance along slice (%1)").arg(plotData.lengthUnit));
  ui->concentrationPlot->yAxis->setLabel(
      plotData.concentrationUnit.isEmpty()
          ? "concentration"
          : QString("concentration (%1)").arg(plotData.concentrationUnit));
  ui->concentrationPlot->legend->setVisible(false);
  if (plotData.simulation == nullptr || slice.empty() ||
      ui->lblSlice->getSlicePixels().empty() || time.isEmpty()) {
    ui->concentrationPlot->replot();
    return;
  }

  const auto &sim{*plotData.simulation};
  const bool showAllSpecies{plotData.speciesToDraw.empty()};
  const bool multipleCompartments{sim.getCompartmentIds().size() > 1};
  bool haveCurrentYRange{false};
  double currentYAxisMin{0.0};
  double currentYAxisMax{0.0};
  for (std::size_t ic = 0; ic < sim.getCompartmentIds().size(); ++ic) {
    std::vector<std::size_t> speciesIndices;
    if (showAllSpecies) {
      speciesIndices.resize(sim.getSpeciesIds(ic).size());
      std::iota(speciesIndices.begin(), speciesIndices.end(), 0);
    } else if (ic < plotData.speciesToDraw.size()) {
      speciesIndices = plotData.speciesToDraw[ic];
    }
    const auto &speciesNames{sim.getPyNames(ic)};
    const auto &speciesColors{sim.getSpeciesColors(ic)};
    for (std::size_t is : speciesIndices) {
      QVector<double> concentration;
      concentration.reserve(static_cast<int>(sliceArrayIndices.size()));
      const auto concArray{sim.getConcArray(
          static_cast<std::size_t>(selectedTimepointIndex), ic, is)};
      for (std::size_t arrayIndex : sliceArrayIndices) {
        const double value{concArray[arrayIndex]};
        concentration.push_back(value);
        if (!haveCurrentYRange) {
          currentYAxisMin = value;
          currentYAxisMax = value;
          haveCurrentYRange = true;
        } else {
          currentYAxisMin = std::min(currentYAxisMin, value);
          currentYAxisMax = std::max(currentYAxisMax, value);
        }
      }
      auto *graph{ui->concentrationPlot->addGraph()};
      graph->setPen(QColor(speciesColors[is]));
      QString name{QString::fromStdString(speciesNames[is])};
      if (multipleCompartments) {
        QString compartmentName{
            ic < static_cast<std::size_t>(plotData.compartmentNames.size())
                ? plotData.compartmentNames[static_cast<int>(ic)]
                : QString::fromStdString(sim.getCompartmentIds()[ic])};
        name = QString("%1 (%2)").arg(name, compartmentName);
      }
      graph->setName(name);
      graph->setData(distanceAlongSlice, concentration, true);
    }
  }

  ui->concentrationPlot->legend->setVisible(
      ui->concentrationPlot->graphCount() > 0);
  if (ui->concentrationPlot->graphCount() > 0) {
    setAxisRange(ui->concentrationPlot->xAxis, distanceAlongSlice.front(),
                 distanceAlongSlice.back());
    if (ui->chkAutoscaleYAxis->isChecked()) {
      setAxisRange(ui->concentrationPlot->yAxis, currentYAxisMin,
                   currentYAxisMax);
    } else {
      if (!fixedAxisRangesValid) {
        updateFixedAxisRanges();
      }
      setAxisRange(ui->concentrationPlot->yAxis, fixedYAxisMin, fixedYAxisMax);
    }
  }
  ui->concentrationPlot->replot();
}

void DialogImageSlice::saveSlicedImage() {
  QString filename = QFileDialog::getSaveFileName(this, "Save sliced image", "",
                                                  "PNG (*.png)");
  if (filename.isEmpty()) {
    return;
  }
  if (filename.right(4) != ".png") {
    filename.append(".png");
  }
  getSlicedImage().save(filename);
}

void DialogImageSlice::cmbSliceType_activated(int index) {
  if (index == 2) {
    sliceType = SliceType::Custom;
    lblSlice_sliceDrawn(startPoint, endPoint);
    return;
  }
  if (index == 0) {
    sliceType = SliceType::Horizontal;
  } else if (index == 1) {
    sliceType = SliceType::Vertical;
  }
  lblSlice_mouseDown(
      QPoint(imgs[0].volume().width() / 2, imgs[0].volume().height() / 2));
}

QString
DialogImageSlice::formatPhysicalPoint(const sme::common::Voxel &voxel) const {
  if (plotData.geometry != nullptr) {
    return plotData.geometry->getPhysicalPointAsString(voxel);
  }
  return QString("x: %1, y: %2, z: %3")
      .arg(voxel.p.x())
      .arg(voxel.p.y())
      .arg(voxel.z);
}

void DialogImageSlice::lblSlice_mouseDown(QPoint point) {
  if (sliceType == SliceType::Horizontal) {
    horizontal = point.y();
    ui->lblSlice->setHorizontalSlice(horizontal);
    updateSlicedImage();
  } else if (sliceType == SliceType::Vertical) {
    vertical = point.x();
    ui->lblSlice->setVerticalSlice(vertical);
    updateSlicedImage();
  }
}

void DialogImageSlice::lblSlice_sliceDrawn(QPoint start, QPoint end) {
  if (sliceType == SliceType::Custom) {
    startPoint = start;
    endPoint = end;
    ui->lblSlice->setSlice(startPoint, endPoint);
    updateSlicedImage();
  }
}

void DialogImageSlice::lblSlice_mouseWheelEvent(int delta) {
  int dp = delta / std::abs(delta);
  QPoint p(std::clamp(horizontal + dp, 0, imgs[0].volume().height() - 1),
           std::clamp(vertical + dp, 0, imgs[0].volume().width() - 1));
  lblSlice_mouseDown(p);
}

void DialogImageSlice::lblSlice_mouseOver(QPoint point) {
  ui->lblMouseLocation->setText(
      QString("Mouse location: %1")
          .arg(formatPhysicalPoint({point.x(), point.y(), zIndex})));
}

void DialogImageSlice::lblImage_mouseOver(const sme::common::Voxel &voxel) {
  if (time.isEmpty() || slice.empty() ||
      ui->lblSlice->getSlicePixels().empty()) {
    return;
  }
  if (voxel.p.x() < 0 || voxel.p.x() >= static_cast<int>(time.size())) {
    return;
  }
  auto i{static_cast<std::size_t>(slice[0].height() - 1 - voxel.p.y())};
  if (i >= ui->lblSlice->getSlicePixels().size()) {
    return;
  }
  const auto &p = ui->lblSlice->getSlicePixels()[i];
  ui->lblMouseLocation->setText(
      QString("Mouse location: %1, t: %2")
          .arg(formatPhysicalPoint({p.x(), p.y(), zIndex}))
          .arg(formatValueWithUnit(time[voxel.p.x()], plotData.timeUnit)));
}

void DialogImageSlice::slideTimepoint_valueChanged(int value) {
  selectedTimepointIndex = value;
  updateSelectedTimepointText();
  ui->lblImage->setVerticalIndicatorSourceX(selectedTimepointIndex);
  updateConcentrationPlot();
}

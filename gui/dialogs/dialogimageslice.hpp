#pragma once

#include "sme/image_stack.hpp"
#include <QDialog>
#include <QStringList>
#include <memory>
#include <vector>

namespace Ui {
class DialogImageSlice;
}

namespace sme::simulate {
class Simulation;
}

enum class SliceType { Horizontal, Vertical, Custom };

struct DialogImageSlicePlotData {
  const sme::simulate::Simulation *simulation{nullptr};
  QStringList compartmentNames{};
  std::vector<std::vector<std::size_t>> speciesToDraw{};
  QString timeUnit{};
  QString lengthUnit{};
  QString concentrationUnit{};
  int timepointIndex{-1};
};

class DialogImageSlice : public QDialog {
  Q_OBJECT

public:
  explicit DialogImageSlice(const sme::common::ImageStack &geometryImage,
                            const QVector<sme::common::ImageStack> &images,
                            const QVector<double> &timepoints, bool invertYAxis,
                            DialogImageSlicePlotData plotData = {},
                            QWidget *parent = nullptr);
  ~DialogImageSlice() override;
  QImage getSlicedImage() const;

private:
  // todo: don't hard-code z to zero here
  static constexpr std::size_t z_index{0};
  const std::unique_ptr<Ui::DialogImageSlice> ui;
  const QVector<sme::common::ImageStack> &imgs;
  const QVector<double> &time;
  const DialogImageSlicePlotData plotData;
  sme::common::ImageStack slice;
  QVector<double> distanceAlongSlice;
  std::vector<std::size_t> sliceArrayIndices;
  SliceType sliceType{SliceType::Vertical};
  int horizontal{0};
  int vertical{0};
  int selectedTimepointIndex{0};
  bool fixedAxisRangesValid{false};
  double fixedXAxisMin{0.0};
  double fixedXAxisMax{1.0};
  double fixedYAxisMin{0.0};
  double fixedYAxisMax{1.0};
  QPoint startPoint{0, 0};
  QPoint endPoint{0, 0};
  void initConcentrationPlot();
  void updateSlicePlotData();
  void updateFixedAxisRanges();
  void updateSlicedImage();
  void updateDisplayedSlicedImage();
  void updateSelectedTimepointText();
  void updateConcentrationPlot();
  void saveSlicedImage();
  void cmbSliceType_activated(int index);
  void lblSlice_mouseDown(QPoint point);
  void lblSlice_sliceDrawn(QPoint start, QPoint end);
  void lblSlice_mouseWheelEvent(int delta);
  void lblSlice_mouseOver(QPoint point);
  void lblImage_mouseOver(const sme::common::Voxel &voxel);
  void slideTimepoint_valueChanged(int value);
};

#pragma once

#include "sme/image_stack.hpp"
#include <QDialog>
#include <memory>

namespace Ui {
class DialogImageSlice;
}

enum class SliceType { Horizontal, Vertical, Custom };

class DialogImageSlice : public QDialog {
  Q_OBJECT

public:
  explicit DialogImageSlice(const sme::common::ImageStack &geometryImage,
                            const QVector<sme::common::ImageStack> &images,
                            const QVector<double> &timepoints, bool invertYAxis,
                            QWidget *parent = nullptr);
  ~DialogImageSlice() override;
  QImage getSlicedImage() const;

private:
  // todo: don't hard-code z to zero here
  static constexpr std::size_t z_index{0};
  const std::unique_ptr<Ui::DialogImageSlice> ui;
  const QVector<sme::common::ImageStack> &imgs;
  const QVector<double> &time;
  sme::common::ImageStack slice;
  SliceType sliceType;
  int horizontal;
  int vertical;
  QPoint startPoint{0, 0};
  QPoint endPoint{0, 0};
  void updateSlicedImage();
  void saveSlicedImage();
  void cmbSliceType_activated(int index);
  void lblSlice_mouseDown(QPoint point);
  void lblSlice_sliceDrawn(QPoint start, QPoint end);
  void lblSlice_mouseWheelEvent(int delta);
  void lblSlice_mouseOver(QPoint point);
  void lblImage_mouseOver(const sme::common::Voxel &voxel);
};

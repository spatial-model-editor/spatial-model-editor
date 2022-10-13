#pragma once

#include "sme/model_units.hpp"
#include <QDialog>
#include <memory>

namespace Ui {
class DialogGeometryImage;
}

class DialogGeometryImage : public QDialog {
  Q_OBJECT

public:
  explicit DialogGeometryImage(const sme::common::ImageStack &image,
                               sme::common::VolumeF voxelSize,
                               const sme::model::ModelUnits &modelUnits,
                               QWidget *parent = nullptr);
  ~DialogGeometryImage() override;
  [[nodiscard]] sme::common::VolumeF getVoxelSize() const;
  [[nodiscard]] bool imageSizeAltered() const;
  [[nodiscard]] bool imageColoursAltered() const;
  [[nodiscard]] const sme::common::ImageStack &getAlteredImage() const;

private:
  std::unique_ptr<Ui::DialogGeometryImage> ui;
  const sme::common::ImageStack &originalImage;
  sme::common::ImageStack coloredImage;
  sme::common::ImageStack rescaledImage;
  bool alteredSize{false};
  bool alteredColours{false};
  bool selectingColours{false};
  QVector<QRgb> colorTable;
  sme::common::VolumeF voxelLocalUnits;
  sme::common::VolumeF voxelModelUnits;
  const sme::model::ModelUnits &units;
  QString modelUnitSymbol;

  void updateVoxelSize();
  void updateColours();
  void enableWidgets(bool enable);
  void lblImage_mouseClicked(QRgb col, sme::common::Voxel voxel);
  void txtImageWidth_editingFinished();
  void txtImageHeight_editingFinished();
  void txtImageDepth_editingFinished();
  void spinPixelsX_valueChanged(int value);
  void spinPixelsY_valueChanged(int value);
  void btnResetPixels_clicked();
  void btnSelectColours_clicked();
  void btnApplyColours_clicked();
  void btnResetColours_clicked();
};

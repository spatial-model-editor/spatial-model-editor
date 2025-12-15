#pragma once

#include "sme/geometry_utils.hpp"
#include "sme/image_stack.hpp"
#include "sme/model.hpp"
#include "sme/utils.hpp"
#include <QDialog>
#include <memory>

namespace Ui {
class DialogImageData;
}

enum class DialogImageDataDataType {
  Concentration,
  ConcentrationRateOfChange,
  DiffusionConstant
};

class DialogImageData : public QDialog {
  Q_OBJECT

public:
  explicit DialogImageData(
      const std::vector<double> &dataArray,
      const sme::model::SpeciesGeometry &speciesGeometry,
      bool invertYAxis = false,
      DialogImageDataDataType dataType = DialogImageDataDataType::Concentration,
      QWidget *parent = nullptr);
  ~DialogImageData() override;
  const std::vector<double> &getData() const;
  [[nodiscard]] std::vector<double> getImageArray() const;

private:
  std::unique_ptr<Ui::DialogImageData> ui;

  // user supplied data
  const sme::common::VolumeF voxelSize;
  const std::vector<sme::common::Voxel> &voxels;
  const sme::common::VoxelF physicalOrigin;
  QString lengthUnit;
  QString quantityUnit;
  QString quantityName;

  QImage colorMaxConc;
  QImage colorMinConc;
  sme::common::ImageStack imgs;
  sme::geometry::VoxelIndexer qpi;
  std::vector<double> data;

  sme::common::VoxelF physicalPoint(const sme::common::Voxel &voxel) const;
  std::size_t pointToArrayIndex(const sme::common::Voxel &voxel) const;
  void importArray(const std::vector<double> &concentrationArray);
  void importImage(const sme::common::ImageStack &concentrationImage);
  void updateImageFromData();
  void rescaleData(double newMin, double newMax);
  void gaussianFilter(const sme::common::Voxel &direction, double sigma);
  void smoothData();
  void lblImage_mouseOver(const sme::common::Voxel &voxel);
  void chkGrid_stateChanged(int state);
  void chkScale_stateChanged(int state);
  void btnImportImage_clicked();
  void btnExportImage_clicked();
  void cmbExampleImages_currentTextChanged(const QString &text);
};

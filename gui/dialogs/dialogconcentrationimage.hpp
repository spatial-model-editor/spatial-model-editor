#pragma once

#include "sme/geometry_utils.hpp"
#include "sme/image_stack.hpp"
#include "sme/model.hpp"
#include "sme/utils.hpp"
#include <QDialog>
#include <memory>

namespace Ui {
class DialogConcentrationImage;
}

class DialogConcentrationImage : public QDialog {
  Q_OBJECT

public:
  explicit DialogConcentrationImage(
      const std::vector<double> &concentrationArray,
      const sme::model::SpeciesGeometry &speciesGeometry,
      bool invertYAxis = false,
      const QString &windowTitle = "Set Initial Concentration Image",
      bool isRateOfChange = false, QWidget *parent = nullptr);
  ~DialogConcentrationImage() override;
  [[nodiscard]] std::vector<double> getConcentrationArray() const;

private:
  std::unique_ptr<Ui::DialogConcentrationImage> ui;

  // user supplied data
  const sme::common::VolumeF voxelSize;
  const std::vector<sme::common::Voxel> &voxels;
  const sme::common::VoxelF physicalOrigin;
  QString lengthUnit;
  QString quantityUnit;
  QString quantityName;

  QImage colourMaxConc;
  QImage colourMinConc;
  sme::common::ImageStack imgs;
  sme::geometry::VoxelIndexer qpi;
  std::vector<double> concentration;

  sme::common::VoxelF physicalPoint(const sme::common::Voxel &voxel) const;
  std::size_t
  pointToConcentrationArrayIndex(const sme::common::Voxel &voxel) const;
  void importConcentrationArray(const std::vector<double> &concentrationArray);
  void
  importConcentrationImage(const sme::common::ImageStack &concentrationImage);
  void updateImageFromConcentration();
  void rescaleConcentration(double newMin, double newMax);
  void gaussianFilter(const sme::common::Voxel &direction, double sigma);
  void smoothConcentration();
  void lblImage_mouseOver(const sme::common::Voxel &voxel);
  void chkGrid_stateChanged(int state);
  void chkScale_stateChanged(int state);
  void btnImportImage_clicked();
  void btnExportImage_clicked();
  void cmbExampleImages_currentTextChanged(const QString &text);
};

#pragma once

#include "sme/geometry_utils.hpp"
#include "sme/image_stack.hpp"
#include "sme/utils.hpp"
#include <QDialog>
#include <map>
#include <memory>
#include <string>
#include <utility>

namespace Ui {
class DialogAnalytic;
}

namespace sme::model {
struct SpeciesGeometry;
class ModelParameters;
class ModelFunctions;
} // namespace sme::model

class DialogAnalytic : public QDialog {
  Q_OBJECT

public:
  explicit DialogAnalytic(const QString &analyticExpression,
                          const sme::model::SpeciesGeometry &speciesGeometry,
                          const sme::model::ModelParameters &modelParameters,
                          const sme::model::ModelFunctions &modelFunctions,
                          bool invertYAxis, QWidget *parent = nullptr);
  ~DialogAnalytic() override;
  const std::string &getExpression() const;
  const QImage &getImage() const;
  bool isExpressionValid() const;

private:
  std::unique_ptr<Ui::DialogAnalytic> ui;
  // user supplied data
  const sme::common::VolumeF voxelSize;
  const std::vector<sme::common::Voxel> &voxels;
  const sme::common::VoxelF physicalOrigin;
  QString lengthUnit;
  QString concentrationUnit;

  sme::common::ImageStack imgs;
  sme::geometry::VoxelIndexer qpi;
  std::vector<double> concentration;
  std::string displayExpression;
  std::string variableExpression;
  bool expressionIsValid = false;

  sme::common::VoxelF physicalPoint(const sme::common::Voxel &voxel) const;
  void txtExpression_mathChanged(const QString &math, bool valid,
                                 const QString &errorMessage);
  void lblImage_mouseOver(const sme::common::Voxel &voxel);
  void btnExportImage_clicked();
};

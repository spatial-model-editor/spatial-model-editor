// TabFeatures

#pragma once

#include <QWidget>
#include <memory>

class QLabelMouseTracker;
class QVoxelRenderer;

namespace Ui {
class TabFeatures;
}

namespace sme::model {
class Model;
} // namespace sme::model

class TabFeatures : public QWidget {
  Q_OBJECT

public:
  explicit TabFeatures(sme::model::Model &m,
                       QLabelMouseTracker *mouseTracker = nullptr,
                       QVoxelRenderer *voxelRenderer = nullptr,
                       QWidget *parent = nullptr);
  ~TabFeatures();
  void loadModelData(const QString &selection = {});

private:
  std::unique_ptr<Ui::TabFeatures> ui;
  sme::model::Model &model;
  QLabelMouseTracker *lblGeometry;
  QVoxelRenderer *voxGeometry;
  int currentFeatureIndex{-1};
  void enableWidgets(bool enable);
  void updateRoiWidgetEnableState();
  void updateRoiImage();
  void updateRegionColors();
  void listFeatures_currentRowChanged(int row);
  void btnAddFeature_clicked();
  void btnRemoveFeature_clicked();
  void txtFeatureName_editingFinished();
  void cmbCompartment_currentIndexChanged(int index);
  void cmbSpecies_currentIndexChanged(int index);
  void cmbRoiType_currentIndexChanged(int index);
  void btnEditExpression_clicked();
  void spnNRegions_valueChanged(int value);
  void btnImportImage_clicked();
  void spnBuiltInThickness_valueChanged(int value);
  void cmbBuiltInAxis_currentIndexChanged(int index);
  void cmbReduction_currentIndexChanged(int index);
};

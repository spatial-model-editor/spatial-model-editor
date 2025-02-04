// TabGeometry

#pragma once

#include "sme/image_stack.hpp"
#include <QWidget>
#include <memory>

class QLabel;
class QListWidgetItem;
class QLabelMouseTracker;
class QVoxelRenderer;
class QStatusBar;

namespace sme::model {
class Model;
} // namespace sme::model

namespace Ui {
class TabGeometry;
}

class TabGeometry : public QWidget {
  Q_OBJECT

public:
  explicit TabGeometry(sme::model::Model &m, QLabelMouseTracker *mouseTracker,
                       QVoxelRenderer *voxelRenderer,
                       QStatusBar *statusBar = nullptr,
                       QWidget *parent = nullptr);
  ~TabGeometry() override;
  void loadModelData(const QString &selection = {});
  void enableTabs();
  void invertYAxis(bool enable);

signals:
  void invalidModelOrNoGeometryImage();
  void modelGeometryChanged();

private:
  std::unique_ptr<Ui::TabGeometry> ui;
  sme::model::Model &model;
  QLabelMouseTracker *lblGeometry;
  QVoxelRenderer *voxGeometry;
  QStatusBar *m_statusBar{};
  bool waitingForCompartmentChoice{false};
  bool membraneSelected{false};

  void lblGeometry_mouseClicked(QRgb col, sme::common::Voxel point);
  void btnAddCompartment_clicked();
  void btnRemoveCompartment_clicked();
  void btnChangeCompartment_clicked();
  void txtCompartmentName_editingFinished();
  void btnChangeCompartmentColour_clicked();
  void tabCompartmentGeometry_currentChanged(int index);
  void lblCompShape_mouseOver(const sme::common::Voxel &point);
  void lblCompBoundary_mouseClicked(QRgb col, sme::common::Voxel point);
  void spinBoundaryIndex_valueChanged(int value);
  void spinMaxBoundaryPoints_valueChanged(int value);
  void spinBoundaryZoom_valueChanged(int value);
  void lblCompMesh_mouseClicked(QRgb col, sme::common::Voxel point);
  void mshCompMesh_mouseClicked(int compartmentIndex);
  void spinMaxTriangleArea_valueChanged(int value);
  void updateBoundaries();
  void updateMesh2d();
  void spinMaxCellVolume_valueChanged(int value);
  void cmbRenderMode_currentIndexChanged(int index);
  void spinMeshZoom_valueChanged(int value);
  void listCompartments_itemSelectionChanged();
  void listCompartments_itemDoubleClicked(QListWidgetItem *item);
  void listMembranes_itemSelectionChanged();
};

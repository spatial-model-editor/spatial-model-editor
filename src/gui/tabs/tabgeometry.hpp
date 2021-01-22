// TabGeometry

#pragma once

#include <QWidget>
#include <memory>

class QLabel;
class QListWidgetItem;
class QLabelMouseTracker;

namespace sme {
namespace model {
class Model;
}
} // namespace sme

namespace Ui {
class TabGeometry;
}

class TabGeometry : public QWidget {
  Q_OBJECT

public:
  explicit TabGeometry(sme::model::Model &m, QLabelMouseTracker *mouseTracker,
                       QLabel *statusBarMsg, QWidget *parent = nullptr);
  ~TabGeometry();
  void loadModelData(const QString &selection = {});
  void enableTabs();

signals:
  void invalidModelOrNoGeometryImage();
  void modelGeometryChanged();

private:
  std::unique_ptr<Ui::TabGeometry> ui;
  sme::model::Model &model;
  QLabelMouseTracker *lblGeometry;
  QLabel *statusBarPermanentMessage;
  bool waitingForCompartmentChoice = false;

  void lblGeometry_mouseClicked(QRgb col, QPoint point);
  void btnAddCompartment_clicked();
  void btnRemoveCompartment_clicked();
  void btnChangeCompartment_clicked();
  void txtCompartmentName_editingFinished();
  void tabCompartmentGeometry_currentChanged(int index);
  void lblCompBoundary_mouseClicked(QRgb col, QPoint point);
  void spinBoundaryIndex_valueChanged(int value);
  void spinMaxBoundaryPoints_valueChanged(int value);
  void lblCompMesh_mouseClicked(QRgb col, QPoint point);
  void spinMaxTriangleArea_valueChanged(int value);
  void listCompartments_itemSelectionChanged();
  void listCompartments_itemDoubleClicked(QListWidgetItem *item);
  void listMembranes_itemSelectionChanged();
};

// QTabGeometry

#pragma once

#include <QWidget>
#include <memory>

class QLabel;
class QListWidgetItem;
class QLabelMouseTracker;

namespace sbml {
class SbmlDocWrapper;
}

namespace Ui {
class QTabGeometry;
}

class QTabGeometry : public QWidget {
  Q_OBJECT

 public:
  explicit QTabGeometry(sbml::SbmlDocWrapper &doc,
                        QLabelMouseTracker *mouseTracker, QLabel *statusBarMsg,
                        QWidget *parent = nullptr);
  ~QTabGeometry();
  void loadModelData(const QString &selection = {});
  void enableTabs(bool enable = true);

 signals:
  void invalidModelOrNoGeometryImage();
  void modelGeometryChanged();

 private:
  std::unique_ptr<Ui::QTabGeometry> ui;
  sbml::SbmlDocWrapper &sbmlDoc;
  QLabelMouseTracker *lblGeometry;
  QLabel *statusBarPermanentMessage;
  bool waitingForCompartmentChoice = false;
  QPixmap lblCompartmentColourPixmap = QPixmap(1, 1);

  void lblGeometry_mouseClicked(QRgb col, QPoint point);
  void btnAddCompartment_clicked();
  void btnRemoveCompartment_clicked();
  void btnChangeCompartment_clicked();
  void btnSetCompartmentSizeFromImage_clicked();
  void tabCompartmentGeometry_currentChanged(int index);
  void lblCompBoundary_mouseClicked(QRgb col, QPoint point);
  void spinBoundaryIndex_valueChanged(int value);
  void spinMaxBoundaryPoints_valueChanged(int value);
  void spinBoundaryWidth_valueChanged(double value);
  void lblCompMesh_mouseClicked(QRgb col, QPoint point);
  void spinMaxTriangleArea_valueChanged(int value);
  void generateMesh(int value = 0);
  void listCompartments_currentRowChanged(int currentRow);
  void listCompartments_itemDoubleClicked(QListWidgetItem *item);
};

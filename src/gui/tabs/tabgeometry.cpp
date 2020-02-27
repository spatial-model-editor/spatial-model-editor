#include "tabgeometry.hpp"

#include <QInputDialog>
#include <QMessageBox>

#include "guiutils.hpp"
#include "logger.hpp"
#include "mesh.hpp"
#include "qlabelmousetracker.hpp"
#include "sbml.hpp"
#include "ui_tabgeometry.h"

TabGeometry::TabGeometry(sbml::SbmlDocWrapper &doc,
                         QLabelMouseTracker *mouseTracker, QLabel *statusBarMsg,
                         QWidget *parent)
    : QWidget(parent),
      ui{std::make_unique<Ui::TabGeometry>()},
      sbmlDoc(doc),
      lblGeometry(mouseTracker),
      statusBarPermanentMessage(statusBarMsg) {
  ui->setupUi(this);
  ui->tabCompartmentGeometry->setCurrentIndex(0);

  connect(lblGeometry, &QLabelMouseTracker::mouseClicked, this,
          &TabGeometry::lblGeometry_mouseClicked);

  connect(ui->btnAddCompartment, &QPushButton::clicked, this,
          &TabGeometry::btnAddCompartment_clicked);

  connect(ui->btnRemoveCompartment, &QPushButton::clicked, this,
          &TabGeometry::btnRemoveCompartment_clicked);

  connect(ui->btnChangeCompartment, &QPushButton::clicked, this,
          &TabGeometry::btnChangeCompartment_clicked);

  connect(ui->txtCompartmentName, &QLineEdit::editingFinished, this,
          &TabGeometry::txtCompartmentName_editingFinished);

  connect(ui->tabCompartmentGeometry, &QTabWidget::currentChanged, this,
          &TabGeometry::tabCompartmentGeometry_currentChanged);

  connect(ui->lblCompBoundary, &QLabelMouseTracker::mouseClicked, this,
          &TabGeometry::lblCompBoundary_mouseClicked);

  connect(ui->lblCompBoundary, &QLabelMouseTracker::mouseWheelEvent, this,
          [this](QWheelEvent *ev) {
            QApplication::sendEvent(ui->spinMaxBoundaryPoints, ev);
          });

  connect(ui->spinBoundaryIndex, qOverload<int>(&QSpinBox::valueChanged), this,
          &TabGeometry::spinBoundaryIndex_valueChanged);

  connect(ui->spinMaxBoundaryPoints, qOverload<int>(&QSpinBox::valueChanged),
          this, &TabGeometry::spinMaxBoundaryPoints_valueChanged);

  connect(ui->spinBoundaryWidth,
          qOverload<double>(&QDoubleSpinBox::valueChanged), this,
          &TabGeometry::spinBoundaryWidth_valueChanged);

  connect(ui->lblCompMesh, &QLabelMouseTracker::mouseClicked, this,
          &TabGeometry::lblCompMesh_mouseClicked);

  connect(ui->lblCompMesh, &QLabelMouseTracker::mouseWheelEvent, this,
          [this](QWheelEvent *ev) {
            QApplication::sendEvent(ui->spinMaxTriangleArea, ev);
          });

  connect(ui->spinMaxTriangleArea, qOverload<int>(&QSpinBox::valueChanged),
          this, &TabGeometry::spinMaxTriangleArea_valueChanged);

  connect(ui->listCompartments, &QListWidget::currentRowChanged, this,
          &TabGeometry::listCompartments_currentRowChanged);

  connect(ui->listCompartments, &QListWidget::itemDoubleClicked, this,
          &TabGeometry::listCompartments_itemDoubleClicked);
}

TabGeometry::~TabGeometry() = default;

void TabGeometry::loadModelData(const QString &selection) {
  ui->tabCompartmentGeometry->setCurrentIndex(0);
  ui->listCompartments->clear();
  ui->lblCompShape->clear();
  ui->lblCompBoundary->clear();
  ui->lblCompMesh->clear();
  ui->lblCompartmentColour->clear();
  ui->txtCompartmentName->clear();
  ui->listCompartments->insertItems(0, sbmlDoc.compartmentNames);
  ui->btnChangeCompartment->setEnabled(false);
  ui->txtCompartmentName->setEnabled(false);
  if (ui->listCompartments->count() > 0) {
    ui->txtCompartmentName->setEnabled(true);
    ui->listCompartments->setCurrentRow(0);
    ui->btnChangeCompartment->setEnabled(true);
  }
  lblGeometry->setImage(sbmlDoc.getCompartmentImage());
  enableTabs(sbmlDoc.hasValidGeometry);
  selectMatchingOrFirstItem(ui->listCompartments, selection);
  // ui->lblGeometryStatus->setText("Compartment Geometry:");
}

void TabGeometry::enableTabs(bool enable) {
  ui->tabCompartmentGeometry->setTabEnabled(1, enable);
  ui->tabCompartmentGeometry->setTabEnabled(2, enable);
}

void TabGeometry::lblGeometry_mouseClicked(QRgb col, QPoint point) {
  if (waitingForCompartmentChoice) {
    SPDLOG_INFO("colour {:x}", col);
    SPDLOG_INFO("point ({},{})", point.x(), point.y());
    // update compartment geometry (i.e. colour) of selected compartment to
    // the one the user just clicked on
    const auto &compartmentID =
        sbmlDoc.compartments.at(ui->listCompartments->currentRow());
    sbmlDoc.setCompartmentColour(compartmentID, col);
    sbmlDoc.setCompartmentInteriorPoint(compartmentID, point);
    ui->tabCompartmentGeometry->setCurrentIndex(0);
    // update display by simulating user click on listCompartments
    listCompartments_currentRowChanged(ui->listCompartments->currentRow());
    enableTabs();
    waitingForCompartmentChoice = false;
    statusBarPermanentMessage->clear();
    enableTabs(sbmlDoc.hasValidGeometry);
    emit modelGeometryChanged();
  } else {
    // display compartment the user just clicked on
    auto compID = sbmlDoc.getCompartmentID(col);
    for (int i = 0; i < sbmlDoc.compartments.size(); ++i) {
      if (sbmlDoc.compartments.at(i) == compID) {
        ui->listCompartments->setCurrentRow(i);
      }
    }
  }
}

void TabGeometry::btnAddCompartment_clicked() {
  bool ok;
  auto compartmentName = QInputDialog::getText(
      this, "Add compartment", "New compartment name:", QLineEdit::Normal, {},
      &ok);
  if (ok && !compartmentName.isEmpty()) {
    QString newCompartmentName = sbmlDoc.addCompartment(compartmentName);
    ui->tabCompartmentGeometry->setCurrentIndex(0);
    enableTabs(false);
    loadModelData(newCompartmentName);
    emit modelGeometryChanged();
  }
}

void TabGeometry::btnRemoveCompartment_clicked() {
  int index = ui->listCompartments->currentRow();
  if (index < 0 || index >= sbmlDoc.compartments.size()) {
    return;
  }
  const auto &compartmentName = ui->listCompartments->item(index)->text();
  const auto &compartmentId = sbmlDoc.compartments.at(index);
  auto msgbox = newYesNoMessageBox(
      "Remove compartment?",
      QString("Remove compartment '%1' from the model?").arg(compartmentName),
      this);
  connect(msgbox, &QMessageBox::finished, this,
          [compartmentId, this](int result) {
            if (result == QMessageBox::Yes) {
              sbmlDoc.removeCompartment(compartmentId);
              ui->tabCompartmentGeometry->setCurrentIndex(0);
              enableTabs(false);
              loadModelData();
              emit modelGeometryChanged();
            }
          });
  msgbox->open();
}

void TabGeometry::btnChangeCompartment_clicked() {
  if (!(sbmlDoc.isValid && sbmlDoc.hasGeometryImage)) {
    emit invalidModelOrNoGeometryImage();
    SPDLOG_DEBUG("invalid geometry and/or model: ignoring");
    return;
  }
  SPDLOG_DEBUG("waiting for user to click on geometry image..");
  waitingForCompartmentChoice = true;
  statusBarPermanentMessage->setText(
      "Please click on the desired location on the compartment geometry "
      "image...");
}

void TabGeometry::txtCompartmentName_editingFinished() {
  int compIndex = ui->listCompartments->currentRow();
  if (compIndex < 0 || compIndex > sbmlDoc.compartments.size() - 1) {
    return;
  }
  if (ui->txtCompartmentName->text() == sbmlDoc.compartmentNames[compIndex]) {
    return;
  }
  const auto &compartmentId = sbmlDoc.compartments.at(compIndex);
  QString name =
      sbmlDoc.setCompartmentName(compartmentId, ui->txtCompartmentName->text());
  ui->txtCompartmentName->setText(name);
  ui->listCompartments->item(compIndex)->setText(name);
}

void TabGeometry::tabCompartmentGeometry_currentChanged(int index) {
  enum TabIndex { IMAGE = 0, BOUNDARIES = 1, MESH = 2 };
  SPDLOG_DEBUG("Tab changed to {} [{}]", index,
               ui->tabCompartmentGeometry->tabText(index).toStdString());
  if (index == TabIndex::BOUNDARIES) {
    auto size = sbmlDoc.mesh->getBoundaries().size();
    ui->spinBoundaryIndex->setMaximum(static_cast<int>(size) - 1);
    spinBoundaryIndex_valueChanged(ui->spinBoundaryIndex->value());
  } else if (index == TabIndex::MESH) {
    auto compIndex =
        static_cast<std::size_t>(ui->listCompartments->currentRow());
    ui->spinMaxTriangleArea->setValue(static_cast<int>(
        sbmlDoc.mesh->getCompartmentMaxTriangleArea(compIndex)));
    spinMaxTriangleArea_valueChanged(ui->spinMaxTriangleArea->value());
  }
}

void TabGeometry::lblCompBoundary_mouseClicked(QRgb col, QPoint point) {
  Q_UNUSED(col);
  Q_UNUSED(point);
  auto index = ui->lblCompBoundary->getMaskIndex();
  if (index <= ui->spinBoundaryIndex->maximum() &&
      index != ui->spinBoundaryIndex->value()) {
    ui->spinBoundaryIndex->setValue(index);
  }
}

void TabGeometry::spinBoundaryIndex_valueChanged(int value) {
  const auto &size = ui->lblCompBoundary->size();
  auto boundaryIndex = static_cast<size_t>(value);
  ui->spinMaxBoundaryPoints->setValue(
      static_cast<int>(sbmlDoc.mesh->getBoundaryMaxPoints(boundaryIndex)));
  ui->lblCompBoundary->setImages(
      sbmlDoc.mesh->getBoundariesImages(size, boundaryIndex));
  if (sbmlDoc.mesh->isMembrane(boundaryIndex)) {
    ui->spinBoundaryWidth->setEnabled(true);
    ui->spinBoundaryWidth->setValue(
        sbmlDoc.mesh->getBoundaryWidth(boundaryIndex));
  } else {
    ui->spinBoundaryWidth->setEnabled(false);
  }
}

void TabGeometry::spinMaxBoundaryPoints_valueChanged(int value) {
  const auto &size = ui->lblCompBoundary->size();
  auto boundaryIndex = static_cast<std::size_t>(ui->spinBoundaryIndex->value());
  sbmlDoc.mesh->setBoundaryMaxPoints(boundaryIndex, static_cast<size_t>(value));
  ui->lblCompBoundary->setImages(
      sbmlDoc.mesh->getBoundariesImages(size, boundaryIndex));
}

void TabGeometry::spinBoundaryWidth_valueChanged(double value) {
  const auto &size = ui->lblCompBoundary->size();
  auto boundaryIndex = static_cast<std::size_t>(ui->spinBoundaryIndex->value());
  sbmlDoc.mesh->setBoundaryWidth(boundaryIndex, value);
  ui->lblCompBoundary->setImages(
      sbmlDoc.mesh->getBoundariesImages(size, boundaryIndex));
}

void TabGeometry::lblCompMesh_mouseClicked(QRgb col, QPoint point) {
  Q_UNUSED(col);
  Q_UNUSED(point);
  auto index = ui->lblCompMesh->getMaskIndex();
  if (index < ui->listCompartments->count() &&
      index != ui->listCompartments->currentRow()) {
    ui->listCompartments->setCurrentRow(index);
    ui->spinMaxTriangleArea->setFocus();
    ui->spinMaxTriangleArea->selectAll();
  }
}

void TabGeometry::spinMaxTriangleArea_valueChanged(int value) {
  const auto &size = ui->lblCompMesh->size();
  auto compIndex = static_cast<std::size_t>(ui->listCompartments->currentRow());
  sbmlDoc.mesh->setCompartmentMaxTriangleArea(compIndex,
                                              static_cast<std::size_t>(value));
  ui->lblCompMesh->setImages(sbmlDoc.mesh->getMeshImages(size, compIndex));
}

void TabGeometry::listCompartments_currentRowChanged(int currentRow) {
  ui->txtCompartmentName->clear();
  if (currentRow < 0 || currentRow >= ui->listCompartments->count()) {
    ui->btnRemoveCompartment->setEnabled(false);
    return;
  }
  const QString &compID = sbmlDoc.compartments.at(currentRow);
  ui->btnRemoveCompartment->setEnabled(true);
  SPDLOG_DEBUG("row {} selected", currentRow);
  SPDLOG_DEBUG("  - Compartment Name: {}",
               ui->listCompartments->currentItem()->text().toStdString());
  SPDLOG_DEBUG("  - Compartment Id: {}", compID.toStdString());
  ui->txtCompartmentName->setText(sbmlDoc.getCompartmentName(compID));
  QRgb col = sbmlDoc.getCompartmentColour(compID);
  SPDLOG_DEBUG("  - Compartment colour {:x} ", col);
  if (col == 0) {
    // null (transparent white) RGB colour: compartment does not have
    // an assigned colour in the image
    ui->lblCompShape->setImage(QImage());
    ui->lblCompartmentColour->setText("none");
    ui->lblCompShape->setImage(QImage());
    ui->lblCompShape->setText(
        "<p>Compartment has no assigned geometry</p> "
        "<ul><li>please click on the 'Select compartment geometry...' "
        "button below</li> "
        "<li> then on the desired location in the geometry "
        "image on the left</li></ul>");
    ui->lblCompMesh->setImage(QImage());
    ui->lblCompMesh->setText("none");
  } else {
    // update colour box
    lblCompartmentColourPixmap.fill(QColor::fromRgb(col));
    ui->lblCompartmentColour->setPixmap(lblCompartmentColourPixmap);
    ui->lblCompartmentColour->setText("");
    // update image of compartment
    ui->lblCompShape->setImage(
        sbmlDoc.mapCompIdToGeometry.at(compID).getCompartmentImage());
    ui->lblCompShape->setText("");
    // update mesh or boundary image if tab is currently visible
    tabCompartmentGeometry_currentChanged(
        ui->tabCompartmentGeometry->currentIndex());
  }
}

void TabGeometry::listCompartments_itemDoubleClicked(QListWidgetItem *item) {
  // double-click on compartment list item is equivalent to
  // selecting item, then clicking on btnChangeCompartment
  if (item != nullptr) {
    btnChangeCompartment_clicked();
  }
}

#include "tabgeometry.hpp"
#include "guiutils.hpp"
#include "qlabelmousetracker.hpp"
#include "sme/logger.hpp"
#include "sme/mesh.hpp"
#include "sme/model.hpp"
#include "ui_tabgeometry.h"
#include <QInputDialog>
#include <QMessageBox>
#include <QStatusBar>
#include <stdexcept>

TabGeometry::TabGeometry(sme::model::Model &m, QLabelMouseTracker *mouseTracker,
                         QStatusBar *statusBar, QWidget *parent)
    : QWidget(parent), ui{std::make_unique<Ui::TabGeometry>()}, model(m),
      lblGeometry(mouseTracker), m_statusBar{statusBar} {
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
  connect(ui->lblCompShape, &QLabelMouseTracker::mouseOver, this,
          &TabGeometry::lblCompShape_mouseOver);
  connect(lblGeometry->getZSlider(), &QSlider::valueChanged, ui->lblCompShape,
          &QLabelMouseTracker::setZIndex);
  connect(ui->lblCompBoundary, &QLabelMouseTracker::mouseClicked, this,
          &TabGeometry::lblCompBoundary_mouseClicked);
  connect(lblGeometry->getZSlider(), &QSlider::valueChanged,
          ui->lblCompBoundary, &QLabelMouseTracker::setZIndex);
  connect(ui->lblCompBoundary, &QLabelMouseTracker::mouseWheelEvent, this,
          [this](QWheelEvent *ev) {
            if (ev->modifiers() == Qt::ShiftModifier) {
              QApplication::sendEvent(ui->spinBoundaryZoom, ev);
            } else {
              QApplication::sendEvent(ui->spinMaxBoundaryPoints, ev);
            }
          });
  connect(ui->spinBoundaryIndex, qOverload<int>(&QSpinBox::valueChanged), this,
          &TabGeometry::spinBoundaryIndex_valueChanged);
  connect(ui->spinMaxBoundaryPoints, qOverload<int>(&QSpinBox::valueChanged),
          this, &TabGeometry::spinMaxBoundaryPoints_valueChanged);
  connect(ui->spinBoundaryZoom, qOverload<int>(&QSpinBox::valueChanged), this,
          &TabGeometry::spinBoundaryZoom_valueChanged);
  connect(ui->lblCompMesh, &QLabelMouseTracker::mouseClicked, this,
          &TabGeometry::lblCompMesh_mouseClicked);
  connect(lblGeometry->getZSlider(), &QSlider::valueChanged, ui->lblCompMesh,
          &QLabelMouseTracker::setZIndex);
  connect(ui->lblCompMesh, &QLabelMouseTracker::mouseWheelEvent, this,
          [this](QWheelEvent *ev) {
            if (ev->modifiers() == Qt::ShiftModifier) {
              QApplication::sendEvent(ui->spinMeshZoom, ev);
            } else {
              QApplication::sendEvent(ui->spinMaxTriangleArea, ev);
            }
          });
  connect(ui->spinMaxTriangleArea, qOverload<int>(&QSpinBox::valueChanged),
          this, &TabGeometry::spinMaxTriangleArea_valueChanged);
  connect(ui->spinMeshZoom, qOverload<int>(&QSpinBox::valueChanged), this,
          &TabGeometry::spinMeshZoom_valueChanged);
  connect(ui->listCompartments, &QListWidget::itemSelectionChanged, this,
          &TabGeometry::listCompartments_itemSelectionChanged);
  connect(ui->listCompartments, &QListWidget::itemDoubleClicked, this,
          &TabGeometry::listCompartments_itemDoubleClicked);
  connect(ui->listMembranes, &QListWidget::itemSelectionChanged, this,
          &TabGeometry::listMembranes_itemSelectionChanged);
}

TabGeometry::~TabGeometry() = default;

void TabGeometry::loadModelData(const QString &selection) {
  ui->tabCompartmentGeometry->setCurrentIndex(0);
  ui->listMembranes->clear();
  ui->listMembranes->addItems(model.getMembranes().getNames());
  ui->listCompartments->clear();
  ui->lblCompShape->clear();
  ui->lblCompBoundary->clear();
  ui->lblCompMesh->clear();
  ui->lblCompartmentColour->clear();
  ui->txtCompartmentName->clear();
  ui->listCompartments->addItems(model.getCompartments().getNames());
  ui->btnChangeCompartment->setEnabled(false);
  ui->txtCompartmentName->setEnabled(false);
  if (ui->listCompartments->count() > 0) {
    ui->txtCompartmentName->setEnabled(true);
    ui->listCompartments->setCurrentRow(0);
    ui->btnChangeCompartment->setEnabled(true);
  }
  lblGeometry->setImage(model.getGeometry().getImages());
  lblGeometry->setPhysicalSize(model.getGeometry().getPhysicalSize(),
                               model.getUnits().getLength().name);
  enableTabs();
  selectMatchingOrFirstItem(ui->listCompartments, selection);
}

void TabGeometry::enableTabs() {
  bool enableBoundaries = model.getGeometry().getIsValid();
  bool enableMesh = model.getGeometry().getMesh() != nullptr &&
                    model.getGeometry().getMesh()->isValid();
  auto *tab = ui->tabCompartmentGeometry;
  tab->setTabEnabled(1, enableBoundaries);
  tab->setTabEnabled(2, enableBoundaries);
  tab->setTabEnabled(3, enableMesh);
  ui->listMembranes->setEnabled(enableBoundaries);
}

void TabGeometry::invertYAxis(bool enable) {
  ui->lblCompShape->invertYAxis(enable);
  ui->lblCompBoundary->invertYAxis(enable);
  ui->lblCompMesh->invertYAxis(enable);
}

void TabGeometry::lblGeometry_mouseClicked(QRgb col, sme::common::Voxel point) {
  if (waitingForCompartmentChoice) {
    SPDLOG_INFO("colour {:x}", col);
    SPDLOG_INFO("point ({},{},{})", point.p.x(), point.p.y(), point.z);
    // update compartment geometry (i.e. colour) of selected compartment to
    // the one the user just clicked on
    QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    const auto &compartmentID =
        model.getCompartments().getIds().at(ui->listCompartments->currentRow());
    model.getCompartments().setColour(compartmentID, col);
    ui->tabCompartmentGeometry->setCurrentIndex(0);
    ui->listMembranes->clear();
    ui->listMembranes->addItems(model.getMembranes().getNames());
    // update display by simulating user click on listCompartments
    listCompartments_itemSelectionChanged();
    waitingForCompartmentChoice = false;
    if (m_statusBar != nullptr) {
      m_statusBar->clearMessage();
    }
    QGuiApplication::restoreOverrideCursor();
    enableTabs();
    emit modelGeometryChanged();
    return;
  }
  // display compartment the user just clicked on
  auto compID = model.getCompartments().getIdFromColour(col);
  for (int i = 0; i < model.getCompartments().getIds().size(); ++i) {
    if (model.getCompartments().getIds().at(i) == compID) {
      ui->listCompartments->setCurrentRow(i);
    }
  }
}

void TabGeometry::btnAddCompartment_clicked() {
  bool ok;
  auto compartmentName = QInputDialog::getText(
      this, "Add compartment", "New compartment name:", QLineEdit::Normal, {},
      &ok);
  if (ok && !compartmentName.isEmpty()) {
    QString newCompartmentName = model.getCompartments().add(compartmentName);
    ui->tabCompartmentGeometry->setCurrentIndex(0);
    enableTabs();
    loadModelData(newCompartmentName);
    emit modelGeometryChanged();
  }
}

void TabGeometry::btnRemoveCompartment_clicked() {
  int index = ui->listCompartments->currentRow();
  if (index < 0 || index >= model.getCompartments().getIds().size()) {
    return;
  }
  const auto &compartmentName = ui->listCompartments->item(index)->text();
  const auto &compartmentId = model.getCompartments().getIds()[index];

  auto result{QMessageBox::question(
      this, "Remove compartment?",
      QString("Remove compartment '%1' from the model?").arg(compartmentName),
      QMessageBox::Yes | QMessageBox::No)};
  if (result == QMessageBox::Yes) {
    ui->listCompartments->clearSelection();
    model.getCompartments().remove(compartmentId);
    ui->tabCompartmentGeometry->setCurrentIndex(0);
    enableTabs();
    loadModelData();
    emit modelGeometryChanged();
  }
}

void TabGeometry::btnChangeCompartment_clicked() {
  if (!(model.getIsValid() && model.getGeometry().getHasImage())) {
    emit invalidModelOrNoGeometryImage();
    SPDLOG_DEBUG("invalid geometry and/or model: ignoring");
    return;
  }
  SPDLOG_DEBUG("waiting for user to click on geometry image..");
  waitingForCompartmentChoice = true;
  if (m_statusBar != nullptr) {
    m_statusBar->showMessage(
        "Please click on the desired location on the compartment geometry "
        "image...");
  }
}

void TabGeometry::txtCompartmentName_editingFinished() {
  if (membraneSelected) {
    int membraneIndex{ui->listMembranes->currentRow()};
    if (membraneIndex < 0 ||
        membraneIndex >= model.getMembranes().getIds().size()) {
      // invalid index
      return;
    }
    if (ui->txtCompartmentName->text() ==
        model.getMembranes().getNames()[membraneIndex]) {
      // name is unchanged
      return;
    }
    const auto &membraneId{model.getMembranes().getIds().at(membraneIndex)};
    QString name{model.getMembranes().setName(membraneId,
                                              ui->txtCompartmentName->text())};
    ui->txtCompartmentName->setText(name);
    ui->listMembranes->item(membraneIndex)->setText(name);
    return;
  }
  // compartment
  int compIndex{ui->listCompartments->currentRow()};
  if (compIndex < 0 || compIndex >= model.getCompartments().getIds().size()) {
    return;
  }
  if (ui->txtCompartmentName->text() ==
      model.getCompartments().getNames()[compIndex]) {
    return;
  }
  const auto &compartmentId{model.getCompartments().getIds().at(compIndex)};
  QString name{model.getCompartments().setName(compartmentId,
                                               ui->txtCompartmentName->text())};
  ui->txtCompartmentName->setText(name);
  ui->listCompartments->item(compIndex)->setText(name);
  // changing a compartment name may change membrane names
  ui->listMembranes->clear();
  ui->listMembranes->addItems(model.getMembranes().getNames());
}

void TabGeometry::tabCompartmentGeometry_currentChanged(int index) {
  enum TabIndex { IMAGE = 0, BOUNDARIES = 1, MESH = 2 };
  SPDLOG_DEBUG("Tab changed to {} [{}]", index,
               ui->tabCompartmentGeometry->tabText(index).toStdString());
  if (index == TabIndex::IMAGE) {
    return;
  }
  if (index == TabIndex::BOUNDARIES) {
    const auto *mesh{model.getGeometry().getMesh()};
    if (mesh == nullptr || mesh->getNumBoundaries() == 0) {
      ui->spinBoundaryIndex->setEnabled(false);
      ui->spinMaxBoundaryPoints->setEnabled(false);
      ui->spinBoundaryZoom->setEnabled(false);
      return;
    }
    ui->spinBoundaryIndex->setMaximum(
        static_cast<int>(mesh->getNumBoundaries()) - 1);
    ui->spinBoundaryIndex->setEnabled(true);
    ui->spinMaxBoundaryPoints->setEnabled(true);
    ui->spinBoundaryZoom->setEnabled(true);
    spinBoundaryIndex_valueChanged(ui->spinBoundaryIndex->value());
    return;
  }
  if (index == TabIndex::MESH) {
    const auto *mesh{model.getGeometry().getMesh()};
    if (mesh == nullptr) {
      ui->spinMaxTriangleArea->setEnabled(false);
      ui->spinMeshZoom->setEnabled(false);
      return;
    }
    ui->spinMaxTriangleArea->setEnabled(true);
    ui->spinMeshZoom->setEnabled(true);
    auto compIndex =
        static_cast<std::size_t>(ui->listCompartments->currentRow());
    ui->spinMaxTriangleArea->setValue(static_cast<int>(
        model.getGeometry().getMesh()->getCompartmentMaxTriangleArea(
            compIndex)));
    spinMaxTriangleArea_valueChanged(ui->spinMaxTriangleArea->value());
    if (!mesh->isValid()) {
      QString msg{"Error: Unable to generate mesh.\n\nImproving the accuracy "
                  "of the boundary lines might help. To do this, click on the "
                  "\"Boundary Lines\" "
                  "tab and increase the maximum number of allowed "
                  "points.\n\nError message: "};
      msg.append(mesh->getErrorMessage().c_str());
      ui->lblCompMesh->setText(msg);
      ui->spinMaxTriangleArea->setEnabled(false);
      ui->spinMeshZoom->setEnabled(false);
      return;
    }
  }
}

void TabGeometry::lblCompShape_mouseOver(const sme::common::Voxel &point) {
  if (m_statusBar != nullptr) {
    m_statusBar->showMessage(
        model.getGeometry().getPhysicalPointAsString(point));
  }
}

void TabGeometry::lblCompBoundary_mouseClicked(QRgb col,
                                               sme::common::Voxel point) {
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
  QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  ui->spinMaxBoundaryPoints->setValue(static_cast<int>(
      model.getGeometry().getMesh()->getBoundaryMaxPoints(boundaryIndex)));
  ui->lblCompBoundary->setImages(
      model.getGeometry().getMesh()->getBoundariesImages(size, boundaryIndex));
  ui->lblCompBoundary->setPhysicalSize(model.getGeometry().getPhysicalSize(),
                                       model.getUnits().getLength().name);
  QGuiApplication::restoreOverrideCursor();
}

void TabGeometry::spinMaxBoundaryPoints_valueChanged(int value) {
  const auto &size = ui->lblCompBoundary->size();
  auto boundaryIndex = static_cast<std::size_t>(ui->spinBoundaryIndex->value());
  QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  model.getGeometry().getMesh()->setBoundaryMaxPoints(
      boundaryIndex, static_cast<size_t>(value));
  ui->lblCompBoundary->setImages(
      model.getGeometry().getMesh()->getBoundariesImages(size, boundaryIndex));
  ui->lblCompBoundary->setPhysicalSize(model.getGeometry().getPhysicalSize(),
                                       model.getUnits().getLength().name);
  QGuiApplication::restoreOverrideCursor();
}

void TabGeometry::spinBoundaryZoom_valueChanged(int value) {
  if (value == 0) {
    // rescale to fit entire scroll region
    ui->scrollBoundaryLines->setWidgetResizable(true);
  } else {
    zoomScrollArea(ui->scrollBoundaryLines, value,
                   ui->lblCompBoundary->getRelativePosition());
  }
  spinBoundaryIndex_valueChanged(ui->spinBoundaryIndex->value());
}

void TabGeometry::lblCompMesh_mouseClicked(QRgb col, sme::common::Voxel point) {
  Q_UNUSED(col);
  Q_UNUSED(point);
  auto index = ui->lblCompMesh->getMaskIndex();
  SPDLOG_TRACE("Point ({},{}), Mask index {}", point.p.x(), point.p.y(), index);
  auto membraneIndex = static_cast<int>(index) - ui->listCompartments->count();
  if (index >= 0 && index < ui->listCompartments->count()) {
    ui->listCompartments->setCurrentRow(index);
    ui->spinMaxTriangleArea->setFocus();
    ui->spinMaxTriangleArea->selectAll();
    return;
  }
  if (membraneIndex >= 0 && membraneIndex < ui->listMembranes->count()) {
    ui->listMembranes->setCurrentRow(membraneIndex);
    return;
  }
}

void TabGeometry::spinMaxTriangleArea_valueChanged(int value) {
  const auto &size = ui->lblCompMesh->size();
  auto compIndex = static_cast<std::size_t>(ui->listCompartments->currentRow());
  QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  model.getGeometry().getMesh()->setCompartmentMaxTriangleArea(
      compIndex, static_cast<std::size_t>(value));
  ui->lblCompMesh->setImages(
      model.getGeometry().getMesh()->getMeshImages(size, compIndex));
  ui->lblCompMesh->setPhysicalSize(model.getGeometry().getPhysicalSize(),
                                   model.getUnits().getLength().name);
  QGuiApplication::restoreOverrideCursor();
}

void TabGeometry::spinMeshZoom_valueChanged(int value) {
  if (value == 0) {
    // rescale to fit entire scroll region
    ui->scrollMesh->setWidgetResizable(true);
  } else {
    zoomScrollArea(ui->scrollMesh, value,
                   ui->lblCompMesh->getRelativePosition());
  }
  spinMaxTriangleArea_valueChanged(ui->spinMaxTriangleArea->value());
}

void TabGeometry::listCompartments_itemSelectionChanged() {
  auto items{ui->listCompartments->selectedItems()};
  if (items.isEmpty()) {
    ui->btnRemoveCompartment->setEnabled(false);
    ui->btnChangeCompartment->setEnabled(false);
    return;
  }
  ui->listMembranes->clearSelection();
  int currentRow{ui->listCompartments->row(items[0])};
  ui->txtCompartmentName->clear();
  ui->lblCompSize->clear();
  membraneSelected = false;
  const QString &compId{model.getCompartments().getIds()[currentRow]};
  ui->btnChangeCompartment->setEnabled(true);
  ui->btnRemoveCompartment->setEnabled(true);
  SPDLOG_DEBUG("row {} selected", currentRow);
  SPDLOG_DEBUG("  - Compartment Name: {}",
               ui->listCompartments->currentItem()->text().toStdString());
  SPDLOG_DEBUG("  - Compartment Id: {}", compId.toStdString());
  ui->txtCompartmentName->setEnabled(true);
  ui->txtCompartmentName->setText(model.getCompartments().getName(compId));
  QRgb col{model.getCompartments().getColour(compId)};
  SPDLOG_DEBUG("  - Compartment colour {:x} ", col);
  if (col == 0) {
    // null (transparent) RGB colour: compartment does not have
    // an assigned colour in the image
    ui->lblCompShape->setImage({});
    ui->lblCompartmentColour->setText("none");
    ui->lblCompShape->setImage({});
    ui->lblCompShape->setText(
        "<p>Compartment has no assigned geometry</p> "
        "<ul><li>please click on the 'Select compartment geometry...' "
        "button below</li> "
        "<li> then on the desired location in the geometry "
        "image on the left</li></ul>");
    ui->lblCompMesh->setImage({});
    ui->lblCompMesh->setText("none");
  } else {
    // update colour box
    QImage img(1, 1, QImage::Format_RGB32);
    img.setPixel(0, 0, col);
    ui->lblCompartmentColour->setPixmap(QPixmap::fromImage(img));
    ui->lblCompartmentColour->setText("");
    // update image of compartment
    const auto *comp{model.getCompartments().getCompartment(compId)};
    ui->lblCompShape->setImage(comp->getCompartmentImages());
    ui->lblCompShape->setPhysicalSize(model.getGeometry().getPhysicalSize(),
                                      model.getUnits().getLength().name);
    ui->lblCompShape->setText("");
    // update mesh or boundary image if tab is currently visible
    tabCompartmentGeometry_currentChanged(
        ui->tabCompartmentGeometry->currentIndex());
    // update compartment volume
    double volume{model.getCompartments().getSize(compId)};
    ui->lblCompSize->setText(QString("Volume: %1 %2 (%3 voxels)")
                                 .arg(QString::number(volume, 'g', 13))
                                 .arg(model.getUnits().getVolume().name)
                                 .arg(comp->nVoxels()));
  }
}

void TabGeometry::listCompartments_itemDoubleClicked(QListWidgetItem *item) {
  // double-click on compartment list item is equivalent to
  // selecting item, then clicking on btnChangeCompartment
  if (item != nullptr) {
    btnChangeCompartment_clicked();
  }
}

void TabGeometry::listMembranes_itemSelectionChanged() {
  auto items{ui->listMembranes->selectedItems()};
  if (items.isEmpty()) {
    return;
  }
  int currentRow{ui->listMembranes->row(items[0])};
  membraneSelected = true;
  ui->listCompartments->clearSelection();
  ui->txtCompartmentName->clear();
  ui->txtCompartmentName->setEnabled(true);
  const QString &membraneId{model.getMembranes().getIds()[currentRow]};
  SPDLOG_DEBUG("row {} selected", currentRow);
  SPDLOG_DEBUG("  - Membrane Name: {}",
               ui->listMembranes->currentItem()->text().toStdString());
  SPDLOG_DEBUG("  - Membrane Id: {}", membraneId.toStdString());
  ui->txtCompartmentName->setText(ui->listMembranes->currentItem()->text());
  // update image
  const auto *m{model.getMembranes().getMembrane(membraneId)};
  ui->lblCompShape->setImage(m->getImages());
  ui->lblCompShape->setPhysicalSize(model.getGeometry().getPhysicalSize(),
                                    model.getUnits().getLength().name);
  auto nVoxelPairs{m->getIndexPairs(sme::geometry::Membrane::X).size() +
                   m->getIndexPairs(sme::geometry::Membrane::Y).size() +
                   m->getIndexPairs(sme::geometry::Membrane::Z).size()};
  // update colour box
  QImage img(1, 2, QImage::Format_RGB32);
  img.setPixel(0, 0, m->getCompartmentA()->getColour());
  img.setPixel(0, 1, m->getCompartmentB()->getColour());
  ui->lblCompartmentColour->setPixmap(QPixmap::fromImage(img));
  ui->lblCompartmentColour->setText("");
  // update membrane length
  double area{model.getMembranes().getSize(membraneId)};
  ui->lblCompSize->setText(QString("Area: %1 %2^2 (%3 voxel faces)")
                               .arg(QString::number(area, 'g', 13))
                               .arg(model.getUnits().getLength().name)
                               .arg(nVoxelPairs));
  if (const auto *mesh{model.getGeometry().getMesh()}; mesh != nullptr) {
    ui->lblCompMesh->setImages(mesh->getMeshImages(
        ui->lblCompMesh->size(),
        static_cast<std::size_t>(currentRow + ui->listCompartments->count())));
    ui->lblCompMesh->setPhysicalSize(model.getGeometry().getPhysicalSize(),
                                     model.getUnits().getLength().name);
  }
}

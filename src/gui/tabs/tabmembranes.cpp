#include "tabmembranes.hpp"

#include "logger.hpp"
#include "sbml.hpp"
#include "ui_tabmembranes.h"

TabMembranes::TabMembranes(sbml::SbmlDocWrapper &doc, QWidget *parent)
    : QWidget(parent), ui{std::make_unique<Ui::TabMembranes>()}, sbmlDoc(doc) {
  ui->setupUi(this);
  connect(ui->listMembranes, &QListWidget::currentRowChanged, this,
          &TabMembranes::listMembranes_currentRowChanged);
}

TabMembranes::~TabMembranes() = default;

void TabMembranes::loadModelData() {
  ui->lblMembraneShape->clear();
  ui->listMembranes->clear();
  ui->listMembranes->addItems(sbmlDoc.membraneNames);
  ui->lblMembraneLength->clear();
  ui->lblMembraneLengthUnits->clear();
  if (ui->listMembranes->count() > 0) {
    ui->listMembranes->setCurrentRow(0);
  }
}

void TabMembranes::listMembranes_currentRowChanged(int currentRow) {
  if (currentRow >= 0 && currentRow < ui->listMembranes->count()) {
    const QString &membraneID = sbmlDoc.membranes.at(currentRow);
    SPDLOG_DEBUG("row {} selected", currentRow);
    SPDLOG_DEBUG("  - Membrane Name: {}",
                 ui->listMembranes->currentItem()->text().toStdString());
    SPDLOG_DEBUG("  - Membrane Id: {}", membraneID.toStdString());
    // update image
    QPixmap pixmap = QPixmap::fromImage(sbmlDoc.getMembraneImage(membraneID));
    ui->lblMembraneShape->setPixmap(pixmap);
    auto nPixels = sbmlDoc.membraneVec[static_cast<std::size_t>(currentRow)]
                       .indexPair.size();
    // update membrane length
    double length = static_cast<double>(nPixels) * sbmlDoc.getPixelWidth();
    ui->lblMembraneLength->setText(QString::number(length, 'g', 13));
    ui->lblMembraneLengthUnits->setText(
        sbmlDoc.getModelUnits().getLength().symbol);
  }
}

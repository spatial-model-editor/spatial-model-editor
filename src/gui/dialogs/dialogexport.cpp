#include "dialogexport.hpp"
#include "logger.hpp"
#include "plotwrapper.hpp"
#include "ui_dialogexport.h"
#include "utils.hpp"
#include <QFile>
#include <QFileDialog>
#include <algorithm>

DialogExport::DialogExport(const QVector<QImage> &images,
                           const PlotWrapper *plotWrapper, int timepoint,
                           QWidget *parent)
    : QDialog(parent), ui{std::make_unique<Ui::DialogExport>()}, imgs{images},
      plot(plotWrapper) {
  ui->setupUi(this);
  for (double t : plotWrapper->getTimepoints()) {
    ui->cmbTimepoint->addItem(utils::dblToQStr(t, 6));
  }
  if (timepoint < 0 || timepoint >= ui->cmbTimepoint->count()) {
    timepoint = 0;
  }
  ui->cmbTimepoint->setCurrentIndex(timepoint);
  connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
          &DialogExport::doExport);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
          &DialogExport::reject);
  connect(ui->radSingleTimepoint, &QRadioButton::toggled, this,
          &DialogExport::radSingleTimepoint_toggled);
}

DialogExport::~DialogExport() = default;

void DialogExport::radSingleTimepoint_toggled(bool checked) {
  if (checked) {
    ui->cmbTimepoint->setEnabled(true);
  } else {
    ui->cmbTimepoint->setEnabled(false);
  }
}

void DialogExport::doExport() {
  if (ui->radSingleTimepoint->isChecked()) {
    return saveImage();
  } else if (ui->radAllTimepoints->isChecked()) {
    return saveImages();
  } else if (ui->radCSV->isChecked()) {
    return saveCSV();
  }
}

void DialogExport::saveImage() {
  QString filename = QFileDialog::getSaveFileName(
      this, "Save concentration image", "", "PNG (*.png)");
  if (filename.isEmpty()) {
    reject();
  }
  if (filename.right(4) != ".png") {
    filename.append(".png");
  }
  imgs.at(ui->cmbTimepoint->currentIndex()).save(filename);
  accept();
}

void DialogExport::saveImages() {
  QString dir =
      QFileDialog::getExistingDirectory(this, "Location to save images");
  if (dir.isEmpty()) {
    reject();
  }
  for (int i = 0; i < imgs.size(); ++i) {
    imgs.at(i).save(QDir(dir).filePath(QString("img%1.png").arg(i)));
  }
  accept();
}

void DialogExport::saveCSV() {
  QString filename = QFileDialog::getSaveFileName(
      this, "Save concentrations as CSV text file", "", "txt (*.txt)");
  if (filename.isEmpty()) {
    reject();
  }
  if (filename.right(4) != ".txt") {
    filename.append(".txt");
  }
  if (QFile f(filename);
      f.open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text)) {
    f.write(plot->getDataAsCSV().toUtf8());
  } else {
    SPDLOG_ERROR("Failed to export CSV file '{}'", filename.toStdString());
  }
  accept();
}

#include "dialogexport.hpp"
#include "logger.hpp"
#include "plotwrapper.hpp"
#include "ui_dialogexport.h"
#include "utils.hpp"
#include <QFile>
#include <QFileDialog>
#include <algorithm>

DialogExport::DialogExport(const QVector<QImage> &images,
                           const PlotWrapper *plotWrapper,
                           sme::model::Model &model,
                           const sme::simulate::Simulation &simulation,
                           int timepoint, QWidget *parent)
    : QDialog(parent), ui{std::make_unique<Ui::DialogExport>()}, imgs{images},
      plot(plotWrapper), m(model), sim(simulation) {
  ui->setupUi(this);
  for (double t : plotWrapper->getTimepoints()) {
    ui->cmbTimepoint->addItem(sme::common::dblToQStr(t, 6));
  }
  if (timepoint < 0 || timepoint >= ui->cmbTimepoint->count()) {
    timepoint = 0;
  }
  ui->cmbTimepoint->setCurrentIndex(timepoint);
  connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
          &DialogExport::doExport);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
          &DialogExport::reject);
  connect(ui->radAllImages, &QRadioButton::toggled, this,
          &DialogExport::radios_toggled);
  connect(ui->radCSV, &QRadioButton::toggled, this,
          &DialogExport::radios_toggled);
  connect(ui->radModel, &QRadioButton::toggled, this,
          &DialogExport::radios_toggled);
  connect(ui->radSingleImage, &QRadioButton::toggled, this,
          &DialogExport::radios_toggled);
}

DialogExport::~DialogExport() = default;

void DialogExport::radios_toggled([[maybe_unused]] bool checked) {
  if (ui->radModel->isChecked() || ui->radSingleImage->isChecked()) {
    ui->cmbTimepoint->setEnabled(true);
  } else {
    ui->cmbTimepoint->setEnabled(false);
  }
}

void DialogExport::doExport() {
  if (ui->radAllImages->isChecked()) {
    return saveImages();
  } else if (ui->radCSV->isChecked()) {
    return saveCSV();
  } else if (ui->radModel->isChecked()) {
    auto timePoint{static_cast<std::size_t>(ui->cmbTimepoint->currentIndex())};
    sim.applyConcsToModel(m, timePoint);
    accept();
  } else if (ui->radSingleImage->isChecked()) {
    return saveImage();
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

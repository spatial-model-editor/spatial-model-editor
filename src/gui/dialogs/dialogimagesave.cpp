#include "dialogimagesave.hpp"
#include "logger.hpp"
#include "ui_dialogimagesave.h"
#include "utils.hpp"
#include <QFileDialog>
#include <algorithm>

DialogImageSave::DialogImageSave(const QVector<QImage> &images,
                                 const QVector<double> &timepoints,
                                 int timepoint, QWidget *parent)
    : QDialog(parent), ui{std::make_unique<Ui::DialogImageSave>()},
      imgs{images}, times{timepoints} {
  ui->setupUi(this);
  for (double t : timepoints) {
    ui->cmbTimepoint->addItem(utils::dblToQStr(t, 6));
  }
  if (timepoint < 0 || timepoint >= ui->cmbTimepoint->count()) {
    timepoint = 0;
  }
  ui->cmbTimepoint->setCurrentIndex(timepoint);
  connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
          &DialogImageSave::saveImages);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
          &DialogImageSave::reject);
  connect(ui->radSingleTimepoint, &QRadioButton::toggled, this,
          &DialogImageSave::radSingleTimepoint_toggled);
}

DialogImageSave::~DialogImageSave() = default;

void DialogImageSave::radSingleTimepoint_toggled(bool checked) {
  if (checked) {
    ui->cmbTimepoint->setEnabled(true);
  } else {
    ui->cmbTimepoint->setEnabled(false);
  }
}

void DialogImageSave::saveImages() {
  if (ui->radSingleTimepoint->isChecked()) {
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
  } else {
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
}

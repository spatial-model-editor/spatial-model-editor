#pragma once

#include "sme/model_geometry.hpp"
#include <QDialog>
#include <memory>

namespace Ui {
class DialogImportAnalyticGeometry;
}

class DialogImportAnalyticGeometry : public QDialog {
  Q_OBJECT

public:
  explicit DialogImportAnalyticGeometry(
      const QString &sbmlFilename, const sme::common::Volume &defaultImageSize,
      QWidget *parent = nullptr);
  ~DialogImportAnalyticGeometry() override;
  [[nodiscard]] sme::common::Volume getImageSize() const;

private:
  std::unique_ptr<Ui::DialogImportAnalyticGeometry> ui;
  QString filename;
  sme::common::Volume defaultSize;

  void updatePreview();
  void btnResetResolution_clicked();
};

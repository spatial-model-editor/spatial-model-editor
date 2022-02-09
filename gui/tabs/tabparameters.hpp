// TabParameters

#pragma once

#include <QWidget>
#include <memory>

namespace Ui {
class TabParameters;
}

namespace sme::model {
class Model;
} // namespace sme::model

class TabParameters : public QWidget {
  Q_OBJECT

public:
  explicit TabParameters(sme::model::Model &m, QWidget *parent = nullptr);
  ~TabParameters();
  void loadModelData(const QString &selection = {});

private:
  std::unique_ptr<Ui::TabParameters> ui;
  sme::model::Model &model;
  QString currentParameterId{};
  void listParameters_currentRowChanged(int row);
  void btnAddParameter_clicked();
  void btnRemoveParameter_clicked();
  void txtParameterName_editingFinished();
  void txtExpression_mathChanged(const QString &math, bool valid,
                                 const QString &errorMessage);
};

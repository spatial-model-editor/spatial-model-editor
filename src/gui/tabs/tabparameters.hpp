// TabParameters

#pragma once

#include <QWidget>
#include <memory>

namespace Ui {
class TabParameters;
}

namespace model {
class Model;
}

class TabParameters : public QWidget {
  Q_OBJECT

 public:
  explicit TabParameters(model::Model &model, QWidget *parent = nullptr);
  ~TabParameters();
  void loadModelData(const QString &selection = {});

 private:
  std::unique_ptr<Ui::TabParameters> ui;
  model::Model &model;
  QString currentParameterId{};
  void listParameters_currentRowChanged(int row);
  void btnAddParameter_clicked();
  void btnRemoveParameter_clicked();
  void txtParameterName_editingFinished();
  void txtExpression_mathChanged(const QString &math, bool valid,
                                 const QString &errorMessage);
};

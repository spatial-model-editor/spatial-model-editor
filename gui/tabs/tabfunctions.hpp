// TabFunctions

#pragma once

#include <QWidget>
#include <memory>

namespace Ui {
class TabFunctions;
}

namespace sme {
namespace model {
class Model;
}
} // namespace sme

class TabFunctions : public QWidget {
  Q_OBJECT

public:
  explicit TabFunctions(sme::model::Model &m, QWidget *parent = nullptr);
  ~TabFunctions();
  void loadModelData(const QString &selection = {});

private:
  std::unique_ptr<Ui::TabFunctions> ui;
  sme::model::Model &model;
  QString currentFunctionId{};
  void clearDisplay();
  void listFunctions_currentRowChanged(int row);
  void btnAddFunction_clicked();
  void btnRemoveFunction_clicked();
  void txtFunctionName_editingFinished();
  void listFunctionParams_currentRowChanged(int row);
  void btnAddFunctionParam_clicked();
  void btnRemoveFunctionParam_clicked();
  void txtFunctionDef_mathChanged(const QString &math, bool valid,
                                  const QString &errorMessage);
};

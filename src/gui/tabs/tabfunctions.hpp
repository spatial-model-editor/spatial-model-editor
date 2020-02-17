// TabFunctions

#pragma once

#include <QWidget>
#include <memory>

namespace Ui {
class TabFunctions;
}

namespace sbml {
class SbmlDocWrapper;
}

class TabFunctions : public QWidget {
  Q_OBJECT

 public:
  explicit TabFunctions(sbml::SbmlDocWrapper &doc, QWidget *parent = nullptr);
  ~TabFunctions();
  void loadModelData(const QString &selection = {});

 private:
  std::unique_ptr<Ui::TabFunctions> ui;
  sbml::SbmlDocWrapper &sbmlDoc;
  void listFunctions_currentRowChanged(int row);
  void btnAddFunction_clicked();
  void btnRemoveFunction_clicked();
  void listFunctionParams_currentRowChanged(int row);
  void btnAddFunctionParam_clicked();
  void btnRemoveFunctionParam_clicked();
  void txtFunctionDef_mathChanged(const QString &math, bool valid,
                                  const QString &errorMessage);
  void btnSaveFunctionChanges_clicked();
};

// QTabFunctions

#pragma once

#include <QWidget>
#include <memory>

namespace Ui {
class QTabFunctions;
}

namespace sbml {
class SbmlDocWrapper;
}

class QTabFunctions : public QWidget {
  Q_OBJECT

 public:
  explicit QTabFunctions(sbml::SbmlDocWrapper &doc, QWidget *parent = nullptr);
  ~QTabFunctions();
  void loadModelData(const QString &selection = {});

 private:
  std::unique_ptr<Ui::QTabFunctions> ui;
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

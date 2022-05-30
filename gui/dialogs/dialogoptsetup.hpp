#pragma once
#include "sme/model.hpp"
#include "sme/optimize_options.hpp"
#include <QDialog>
#include <QListWidgetItem>
#include <memory>

namespace Ui {
class DialogOptSetup;
}

class DialogOptSetup : public QDialog {
  Q_OBJECT

public:
  explicit DialogOptSetup(const sme::model::Model &model,
                          QWidget *parent = nullptr);
  ~DialogOptSetup() override;
  [[nodiscard]] const sme::simulate::OptimizeOptions &
  getOptimizeOptions() const;

private:
  const sme::model::Model &model;
  sme::simulate::OptimizeOptions optimizeOptions;
  std::vector<sme::simulate::OptParam> defaultOptParams;
  std::vector<sme::simulate::OptCost> defaultOptCosts;
  std::unique_ptr<Ui::DialogOptSetup> ui;
  void cmbAlgorithm_currentIndexChanged(int index);
  void spinIslands_valueChanged(int value);
  void spinPopulation_valueChanged(int value);
  void lstTargets_currentRowChanged(int row);
  void lstTargets_itemDoubleClicked(QListWidgetItem *item);
  void btnAddTarget_clicked();
  void btnEditTarget_clicked();
  void btnRemoveTarget_clicked();
  void lstParameters_currentRowChanged(int row);
  void lstParameters_itemDoubleClicked(QListWidgetItem *item);
  void btnAddParameter_clicked();
  void btnEditParameter_clicked();
  void btnRemoveParameter_clicked();
};

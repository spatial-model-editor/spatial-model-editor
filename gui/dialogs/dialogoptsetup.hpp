#pragma once
#include "sme/model.hpp"
#include "sme/optimize_options.hpp"
#include <QDialog>
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
  void lstTargets_currentRowChanged(int row);
  void btnAddTarget_clicked();
  void btnEditTarget_clicked();
  void btnRemoveTarget_clicked();
  void lstParameters_currentRowChanged(int row);
  void btnAddParameter_clicked();
  void btnEditParameter_clicked();
  void btnRemoveParameter_clicked();
};

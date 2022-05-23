#pragma once
#include "sme/model.hpp"
#include "sme/optimize_options.hpp"
#include <QDialog>
#include <QStringList>
#include <memory>

namespace Ui {
class DialogOptCost;
}

class DialogOptCost : public QDialog {
  Q_OBJECT

public:
  explicit DialogOptCost(
      const sme::model::Model &model,
      const std::vector<sme::simulate::OptCost> &defaultOptCosts,
      const sme::simulate::OptCost *initialOptCost = nullptr,
      QWidget *parent = nullptr);
  ~DialogOptCost() override;
  [[nodiscard]] const sme::simulate::OptCost &getOptCost() const;

private:
  const sme::model::Model &model;
  const std::vector<sme::simulate::OptCost> &defaultOptCosts;
  std::unique_ptr<Ui::DialogOptCost> ui;
  sme::simulate::OptCost optCost;
  void cmbSpecies_currentIndexChanged(int index);
  void cmbCostType_currentIndexChanged(int index);
  void cmbDiffType_currentIndexChanged(int index);
  void txtSimulationTime_editingFinished();
  void btnEditTargetValues_clicked();
  void txtWeight_editingFinished();
  void txtEpsilon_editingFinished();
};

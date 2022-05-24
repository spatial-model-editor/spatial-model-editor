#include "dialogoptsetup.hpp"
#include "dialogoptcost.hpp"
#include "dialogoptparam.hpp"
#include "sme/logger.hpp"
#include "ui_dialogoptsetup.h"
#include <QMessageBox>

static QString toQStr(double x) { return QString::number(x, 'g', 14); }

static QString toQStr(sme::simulate::OptCostType optCostType) {
  switch (optCostType) {
  case sme::simulate::OptCostType::Concentration:
    return "Concentration";
  case sme::simulate::OptCostType::ConcentrationDcdt:
    return "Rate of change of concentration";
  default:
    return "";
  }
}

static QString toQStr(sme::simulate::OptCostDiffType optCostDiffType) {
  switch (optCostDiffType) {
  case sme::simulate::OptCostDiffType::Absolute:
    return "absolute";
  case sme::simulate::OptCostDiffType::Relative:
    return "relative";
  default:
    return "";
  }
}

static QString toQStr(const sme::simulate::OptCost &optCost) {
  return QString("%1 [%2 at t=%3, %4 difference]")
      .arg(optCost.name.c_str())
      .arg(toQStr(optCost.optCostType))
      .arg(toQStr(optCost.simulationTime))
      .arg(toQStr(optCost.optCostDiffType));
}

static QString toQStr(const sme::simulate::OptParam &optParam) {
  return QString("%1 [%2, %3]")
      .arg(optParam.name.c_str())
      .arg(optParam.lowerBound)
      .arg(optParam.upperBound);
}

static std::vector<sme::simulate::OptParam>
getDefaultOptParams(const sme::model::Model &model) {
  std::vector<sme::simulate::OptParam> optParams;
  for (const auto &id : model.getParameters().getIds()) {
    double value{model.getParameters().getExpression(id).toDouble()};
    optParams.push_back({sme::simulate::OptParamType::ModelParameter,
                         model.getParameters().getName(id).toStdString(),
                         id.toStdString(), "", value, value});
  }
  for (const auto &reactionLocation :
       model.getReactions().getReactionLocations()) {
    for (const auto &reactionId :
         model.getReactions().getIds(reactionLocation)) {
      for (const auto &parameterId :
           model.getReactions().getParameterIds(reactionId)) {
        auto reactionName{model.getReactions().getName(reactionId)};
        auto parameterName{
            model.getReactions().getParameterName(reactionId, parameterId)};
        double value{
            model.getReactions().getParameterValue(reactionId, parameterId)};
        auto name{
            QString("Reaction '%1' / %2").arg(reactionName).arg(parameterName)};
        optParams.push_back({sme::simulate::OptParamType::ReactionParameter,
                             name.toStdString(), parameterId.toStdString(),
                             reactionId.toStdString(), value, value});
      }
    }
  }
  return optParams;
}

static std::vector<sme::simulate::OptCost>
getDefaultOptCosts(const sme::model::Model &model) {
  std::vector<sme::simulate::OptCost> optCosts;
  std::size_t compartmentIndex{0};
  double defaultSimTime{100.0};
  if (const auto &times{model.getSimulationSettings().times}; !times.empty()) {
    defaultSimTime =
        static_cast<double>(times.front().first) * times.front().second;
  }
  for (const auto &compartmentId : model.getCompartments().getIds()) {
    // note: compartment/species indices here are constructed to match those in
    // the simulation data (i.e. constant species are ignored). todo: refactor
    // this logic which is duplicated in the simulation setup
    bool compartmentHasSpecies{false};
    std::size_t speciesIndex{0};
    for (const auto &speciesId : model.getSpecies().getIds(compartmentId)) {
      if (!model.getSpecies().getIsConstant(speciesId)) {
        optCosts.push_back(
            {sme::simulate::OptCostType::Concentration,
             sme::simulate::OptCostDiffType::Absolute,
             QString("%1/%2")
                 .arg(model.getCompartments().getName(compartmentId))
                 .arg(model.getSpecies().getName(speciesId))
                 .toStdString(),
             speciesId.toStdString(),
             defaultSimTime,
             1.0,
             compartmentIndex,
             speciesIndex,
             {},
             1e-14});
        compartmentHasSpecies = true;
        ++speciesIndex;
      }
    }
    if (compartmentHasSpecies) {
      ++compartmentIndex;
    }
  }
  return optCosts;
}

DialogOptSetup::DialogOptSetup(const sme::model::Model &model, QWidget *parent)
    : QDialog(parent), model{model},
      optimizeOptions{model.getOptimizeOptions()},
      defaultOptParams{getDefaultOptParams(model)},
      defaultOptCosts{getDefaultOptCosts(model)},
      ui{std::make_unique<Ui::DialogOptSetup>()} {
  ui->setupUi(this);
  ui->btnEditTarget->setEnabled(false);
  ui->btnRemoveTarget->setEnabled(false);
  ui->btnEditParameter->setEnabled(false);
  ui->btnRemoveParameter->setEnabled(false);
  for (const auto &optCost : optimizeOptions.optCosts) {
    ui->lstTargets->addItem(toQStr(optCost));
  }
  for (const auto &optParam : optimizeOptions.optParams) {
    ui->lstParameters->addItem(toQStr(optParam));
  }
  connect(ui->lstTargets, &QListWidget::currentRowChanged, this,
          &DialogOptSetup::lstTargets_currentRowChanged);
  connect(ui->lstTargets, &QListWidget::itemDoubleClicked, this,
          &DialogOptSetup::lstTargets_itemDoubleClicked);
  connect(ui->btnAddTarget, &QPushButton::clicked, this,
          &DialogOptSetup::btnAddTarget_clicked);
  connect(ui->btnEditTarget, &QPushButton::clicked, this,
          &DialogOptSetup::btnEditTarget_clicked);
  connect(ui->btnRemoveTarget, &QPushButton::clicked, this,
          &DialogOptSetup::btnRemoveTarget_clicked);
  connect(ui->lstParameters, &QListWidget::currentRowChanged, this,
          &DialogOptSetup::lstParameters_currentRowChanged);
  connect(ui->lstParameters, &QListWidget::itemDoubleClicked, this,
          &DialogOptSetup::lstParameters_itemDoubleClicked);
  connect(ui->btnAddParameter, &QPushButton::clicked, this,
          &DialogOptSetup::btnAddParameter_clicked);
  connect(ui->btnEditParameter, &QPushButton::clicked, this,
          &DialogOptSetup::btnEditParameter_clicked);
  connect(ui->btnRemoveParameter, &QPushButton::clicked, this,
          &DialogOptSetup::btnRemoveParameter_clicked);
  connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
          &DialogOptSetup::accept);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
          &DialogOptSetup::reject);
}

DialogOptSetup::~DialogOptSetup() = default;

[[nodiscard]] const sme::simulate::OptimizeOptions &
DialogOptSetup::getOptimizeOptions() const {
  return optimizeOptions;
}

void DialogOptSetup::lstTargets_currentRowChanged(int row) {
  if ((row < 0) || (row > ui->lstTargets->count() - 1)) {
    ui->btnEditTarget->setEnabled(false);
    ui->btnRemoveTarget->setEnabled(false);
    return;
  }
  ui->btnEditTarget->setEnabled(true);
  ui->btnRemoveTarget->setEnabled(true);
}

void DialogOptSetup::lstTargets_itemDoubleClicked(QListWidgetItem *item) {
  if (item != nullptr) {
    btnEditTarget_clicked();
  }
}

void DialogOptSetup::btnAddTarget_clicked() {
  DialogOptCost dia(model, defaultOptCosts);
  if (dia.exec() == QDialog::Accepted) {
    optimizeOptions.optCosts.push_back(dia.getOptCost());
    ui->lstTargets->addItem(toQStr(dia.getOptCost()));
  }
}

void DialogOptSetup::btnEditTarget_clicked() {
  auto row{ui->lstTargets->currentRow()};
  if ((row < 0) || (row > ui->lstTargets->count() - 1)) {
    return;
  }
  DialogOptCost dia(model, defaultOptCosts, &optimizeOptions.optCosts[row]);
  if (dia.exec() == QDialog::Accepted) {
    optimizeOptions.optCosts[row] = dia.getOptCost();
    ui->lstTargets->item(row)->setText(toQStr(dia.getOptCost()));
  }
}

void DialogOptSetup::btnRemoveTarget_clicked() {
  auto row{ui->lstTargets->currentRow()};
  if ((row < 0) || (row > ui->lstTargets->count() - 1)) {
    SPDLOG_TRACE("Invalid row index {}", row);
    return;
  }
  auto result{QMessageBox::question(this, "Remove target?",
                                    QString("Remove optimization target '%1'?")
                                        .arg(ui->lstTargets->item(row)->text()),
                                    QMessageBox::Yes | QMessageBox::No)};
  if (result == QMessageBox::Yes) {
    SPDLOG_TRACE("Removing row {}", row);
    optimizeOptions.optCosts.erase(optimizeOptions.optCosts.begin() + row);
    delete ui->lstTargets->takeItem(row);
  }
}

void DialogOptSetup::lstParameters_currentRowChanged(int row) {
  if ((row < 0) || (row > ui->lstParameters->count() - 1)) {
    ui->btnEditParameter->setEnabled(false);
    ui->btnRemoveParameter->setEnabled(false);
    return;
  }
  ui->btnEditParameter->setEnabled(true);
  ui->btnRemoveParameter->setEnabled(true);
}

void DialogOptSetup::lstParameters_itemDoubleClicked(QListWidgetItem *item) {
  if (item != nullptr) {
    btnEditParameter_clicked();
  }
}

void DialogOptSetup::btnAddParameter_clicked() {
  DialogOptParam dia(defaultOptParams);
  if (dia.exec() == QDialog::Accepted) {
    optimizeOptions.optParams.push_back(dia.getOptParam());
    ui->lstParameters->addItem(toQStr(dia.getOptParam()));
  }
}

void DialogOptSetup::btnEditParameter_clicked() {
  auto row{ui->lstParameters->currentRow()};
  if ((row < 0) || (row > ui->lstParameters->count() - 1)) {
    return;
  }
  DialogOptParam dia(defaultOptParams, &optimizeOptions.optParams[row]);
  if (dia.exec() == QDialog::Accepted) {
    optimizeOptions.optParams[row] = dia.getOptParam();
    ui->lstParameters->item(row)->setText(toQStr(dia.getOptParam()));
  }
}

void DialogOptSetup::btnRemoveParameter_clicked() {
  auto row{ui->lstParameters->currentRow()};
  SPDLOG_TRACE("Removing row {}", row);
  if ((row < 0) || (row > ui->lstParameters->count() - 1)) {
    SPDLOG_TRACE("Invalid row index {}", row);
    return;
  }
  auto result{
      QMessageBox::question(this, "Remove parameter?",
                            QString("Remove optimization parameter '%1'?")
                                .arg(ui->lstParameters->item(row)->text()),
                            QMessageBox::Yes | QMessageBox::No)};
  if (result == QMessageBox::Yes) {
    SPDLOG_TRACE("Removing row {}", row);
    optimizeOptions.optParams.erase(optimizeOptions.optParams.begin() + row);
    delete ui->lstParameters->takeItem(row);
  }
}

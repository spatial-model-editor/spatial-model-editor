#include "dialogdisplayoptions.hpp"
#include "plotwrapper.hpp"
#include "ui_dialogdisplayoptions.h"
#include <QInputDialog>

static void checkItem(QTreeWidgetItem *item, bool isChecked) {
  if (isChecked) {
    item->setCheckState(0, Qt::CheckState::Checked);
  } else {
    item->setCheckState(0, Qt::CheckState::Unchecked);
  }
}

static int boolToIndex(bool value) {
  if (value) {
    return 1;
  }
  return 0;
}

static bool indexToBool(int value) {
  if (value == 1) {
    return true;
  }
  return false;
}

DialogDisplayOptions::DialogDisplayOptions(
    const QStringList &compartmentNames,
    const std::vector<QStringList> &speciesNames,
    const sme::model::DisplayOptions &displayOptions,
    const std::vector<PlotWrapperObservable> &plotWrapperObservables,
    QWidget *parent)
    : QDialog(parent), ui{std::make_unique<Ui::DialogDisplayOptions>()},
      nSpecies(displayOptions.showSpecies.size()),
      observables(plotWrapperObservables) {
  ui->setupUi(this);

  auto *ls = ui->listSpecies;
  ls->clear();
  auto iterChecked = displayOptions.showSpecies.cbegin();
  for (int iComp = 0; iComp < compartmentNames.size(); ++iComp) {
    auto *comp = new QTreeWidgetItem(ls, {compartmentNames[iComp]});
    comp->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable |
                   Qt::ItemIsEnabled | Qt::ItemIsAutoTristate);
    ui->listSpecies->addTopLevelItem(comp);
    for (const auto &speciesName :
         speciesNames[static_cast<std::size_t>(iComp)]) {
      auto *spec = new QTreeWidgetItem(comp, QStringList({speciesName}));
      spec->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable |
                     Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);
      checkItem(spec, *iterChecked);
      comp->addChild(spec);
      ++iterChecked;
    }
  }
  ls->expandAll();
  ui->chkShowMinMaxRanges->setChecked(displayOptions.showMinMax);
  for (const auto &observable : observables) {
    auto *obs =
        new QTreeWidgetItem(ui->listObservables, {observable.expression});
    obs->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable |
                  Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);
    checkItem(obs, observable.visible);
    ui->listObservables->addTopLevelItem(obs);
  }
  ui->cmbNormaliseOverAllTimepoints->setCurrentIndex(
      boolToIndex(displayOptions.normaliseOverAllTimepoints));
  ui->cmbNormaliseOverAllSpecies->setCurrentIndex(
      boolToIndex(displayOptions.normaliseOverAllSpecies));

  connect(ui->listObservables, &QTreeWidget::currentItemChanged, this,
          &DialogDisplayOptions::listObservables_currentItemChanged);
  connect(ui->listObservables, &QTreeWidget::itemDoubleClicked, this,
          &DialogDisplayOptions::btnEditObservable_clicked);
  connect(ui->btnAddObservable, &QPushButton::clicked, this,
          &DialogDisplayOptions::btnAddObservable_clicked);
  connect(ui->btnEditObservable, &QPushButton::clicked, this,
          &DialogDisplayOptions::btnEditObservable_clicked);
  connect(ui->btnRemoveObservable, &QPushButton::clicked, this,
          &DialogDisplayOptions::btnRemoveObservable_clicked);
  connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
          &DialogDisplayOptions::accept);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
          &DialogDisplayOptions::reject);
}

DialogDisplayOptions::~DialogDisplayOptions() = default;

std::vector<bool> DialogDisplayOptions::getShowSpecies() const {
  std::vector<bool> checked;
  for (int ic = 0; ic < ui->listSpecies->topLevelItemCount(); ++ic) {
    const auto *comp = ui->listSpecies->topLevelItem(ic);
    for (int is = 0; is < comp->childCount(); ++is) {
      checked.push_back(comp->child(is)->checkState(0) ==
                        Qt::CheckState::Checked);
    }
  }
  return checked;
}

bool DialogDisplayOptions::getShowMinMax() const {
  return ui->chkShowMinMaxRanges->isChecked();
}

bool DialogDisplayOptions::getNormaliseOverAllTimepoints() const {
  return indexToBool(ui->cmbNormaliseOverAllTimepoints->currentIndex());
}

bool DialogDisplayOptions::getNormaliseOverAllSpecies() const {
  return indexToBool(ui->cmbNormaliseOverAllSpecies->currentIndex());
}

const std::vector<PlotWrapperObservable> &
DialogDisplayOptions::getObservables() {
  for (int i = 0; i < ui->listObservables->topLevelItemCount(); ++i) {
    const auto *item{ui->listObservables->topLevelItem(i)};
    bool isChecked{item->checkState(0) == Qt::CheckState::Checked};
    observables[static_cast<std::size_t>(i)].visible = isChecked;
  }
  return observables;
}

void DialogDisplayOptions::listObservables_currentItemChanged(
    QTreeWidgetItem *current, QTreeWidgetItem *previous) {
  Q_UNUSED(previous);
  bool enable = current != nullptr;
  ui->btnEditObservable->setEnabled(enable);
  ui->btnRemoveObservable->setEnabled(enable);
}

void DialogDisplayOptions::btnAddObservable_clicked() {
  bool ok;
  QString expr = QInputDialog::getText(
      this, "Add new observable", "Expression: ", QLineEdit::Normal, {}, &ok);
  if (!ok || expr.isEmpty()) {
    return;
  }
  auto *obs = new QTreeWidgetItem(ui->listObservables, {expr});
  obs->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable |
                Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);
  checkItem(obs, true);
  ui->listObservables->addTopLevelItem(obs);
  observables.push_back({expr, expr, true});
}

void DialogDisplayOptions::btnEditObservable_clicked() {
  auto *item = ui->listObservables->currentItem();
  if (item == nullptr) {
    return;
  }
  bool ok;
  QString expr = QInputDialog::getText(this, "Edit observable expression",
                                       "Expression: ", QLineEdit::Normal,
                                       item->text(0), &ok);
  if (!ok || expr.isEmpty()) {
    return;
  }
  item->setText(0, expr);
  auto index = ui->listObservables->indexOfTopLevelItem(item);
  auto &obs = observables[static_cast<std::size_t>(index)];
  obs.expression = expr;
  obs.name = expr;
}

void DialogDisplayOptions::btnRemoveObservable_clicked() {
  auto *item = ui->listObservables->currentItem();
  if (item == nullptr) {
    return;
  }
  auto index = ui->listObservables->indexOfTopLevelItem(item);
  delete item;
  observables.erase(observables.begin() + index);
}

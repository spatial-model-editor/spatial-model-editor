#include "dialogdisplayoptions.hpp"

#include "ui_dialogdisplayoptions.h"

static void checkItem(QTreeWidgetItem* item, bool isChecked) {
  if (isChecked) {
    item->setCheckState(0, Qt::CheckState::Checked);
  } else {
    item->setCheckState(0, Qt::CheckState::Unchecked);
  }
}

DialogDisplayOptions::DialogDisplayOptions(
    const QStringList& compartmentNames,
    const std::vector<QStringList>& speciesNames,
    const std::vector<bool>& showSpecies, bool showMinMax, int normalisation,
    QWidget* parent)
    : QDialog(parent),
      ui{std::make_unique<Ui::DialogDisplayOptions>()},
      nSpecies(showSpecies.size()) {
  ui->setupUi(this);

  auto* ls = ui->listSpecies;
  ls->clear();
  auto iterChecked = showSpecies.cbegin();
  for (int iComp = 0; iComp < compartmentNames.size(); ++iComp) {
    QTreeWidgetItem* comp = new QTreeWidgetItem(ls, {compartmentNames[iComp]});
    comp->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable |
                   Qt::ItemIsEnabled | Qt::ItemIsAutoTristate);
    ui->listSpecies->addTopLevelItem(comp);
    for (const auto& speciesName :
         speciesNames[static_cast<std::size_t>(iComp)]) {
      QTreeWidgetItem* spec =
          new QTreeWidgetItem(comp, QStringList({speciesName}));
      spec->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable |
                     Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);
      checkItem(spec, *iterChecked);
      comp->addChild(spec);
      ++iterChecked;
    }
  }
  ls->expandAll();
  ui->chkShowMinMaxRanges->setChecked(showMinMax);
  ui->cmbNormalisation->setCurrentIndex(normalisation);

  connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
          &DialogDisplayOptions::accept);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
          &DialogDisplayOptions::reject);
}

DialogDisplayOptions::~DialogDisplayOptions() = default;

std::vector<bool> DialogDisplayOptions::getShowSpecies() const {
  std::vector<bool> checked;
  for (int ic = 0; ic < ui->listSpecies->topLevelItemCount(); ++ic) {
    const auto* comp = ui->listSpecies->topLevelItem(ic);
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

int DialogDisplayOptions::getNormalisationType() const {
  return ui->cmbNormalisation->currentIndex();
}

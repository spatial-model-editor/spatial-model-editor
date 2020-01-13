// QTabSpecies

#pragma once

#include <QWidget>
#include <memory>

class QTreeWidgetItem;

namespace Ui {
class QTabSpecies;
}

namespace sbml {
class SbmlDocWrapper;
}

class QLabelMouseTracker;

class QTabSpecies : public QWidget {
  Q_OBJECT

 public:
  explicit QTabSpecies(sbml::SbmlDocWrapper &doc,
                       QLabelMouseTracker *mouseTracker,
                       QWidget *parent = nullptr);
  ~QTabSpecies();
  void loadModelData(const QString &selection = {});

 private:
  std::unique_ptr<Ui::QTabSpecies> ui;
  sbml::SbmlDocWrapper &sbmlDoc;
  QLabelMouseTracker *lblGeometry;
  QPixmap lblSpeciesColourPixmap = QPixmap(1, 1);
  QString currentSpeciesId;
  void enableWidgets(bool enable);
  void listSpecies_currentItemChanged(QTreeWidgetItem *current,
                                      QTreeWidgetItem *previous);
  void btnAddSpecies_clicked();
  void btnRemoveSpecies_clicked();
  void txtSpeciesName_editingFinished();
  void cmbSpeciesCompartment_activated(int index);
  void chkSpeciesIsSpatial_toggled(bool enabled);
  void chkSpeciesIsConstant_toggled(bool enabled);
  void radInitialConcentration_toggled();
  void txtInitialConcentration_editingFinished();
  void btnEditAnalyticConcentration_clicked();
  void btnEditImageConcentration_clicked();
  void txtDiffusionConstant_editingFinished();
  void btnChangeSpeciesColour_clicked();
};

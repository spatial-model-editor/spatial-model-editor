#pragma once

#include <QDialog>
#include <memory>

namespace Ui {
class DialogDisplayOptions;
}

struct PlotWrapperObservable;
class QTreeWidgetItem;

class DialogDisplayOptions : public QDialog {
  Q_OBJECT

public:
  explicit DialogDisplayOptions(
      const QStringList &compartmentNames,
      const std::vector<QStringList> &speciesNames,
      const std::vector<bool> &showSpecies, bool showMinMax,
      bool normaliseOverAllTimepoints, bool normaliseOverAllSpecies,
      const std::vector<PlotWrapperObservable> &plotWrapperObservables,
      QWidget *parent = nullptr);
  ~DialogDisplayOptions();
  std::vector<bool> getShowSpecies() const;
  bool getShowMinMax() const;
  bool getNormaliseOverAllTimepoints() const;
  bool getNormaliseOverAllSpecies() const;
  const std::vector<PlotWrapperObservable> &getObservables();

private:
  std::unique_ptr<Ui::DialogDisplayOptions> ui;
  std::size_t nSpecies;
  std::vector<PlotWrapperObservable> observables;
  void listObservables_currentItemChanged(QTreeWidgetItem *current,
                                          QTreeWidgetItem *previous);
  void btnAddObservable_clicked();
  void btnEditObservable_clicked();
  void btnRemoveObservable_clicked();
};

#pragma once

#include <QDialog>
#include <memory>

namespace Ui {
class DialogDisplayOptions;
}

class DialogDisplayOptions : public QDialog {
  Q_OBJECT

public:
  explicit DialogDisplayOptions(const QStringList &compartmentNames,
                                const std::vector<QStringList> &speciesNames,
                                const std::vector<bool> &showSpecies,
                                bool showMinMax,
                                bool normaliseOverAllTimepoints,
                                bool normaliseOverAllSpecies,
                                QWidget *parent = nullptr);
  ~DialogDisplayOptions();
  std::vector<bool> getShowSpecies() const;
  bool getShowMinMax() const;
  bool getNormaliseOverAllTimepoints() const;
  bool getNormaliseOverAllSpecies() const;

private:
  std::unique_ptr<Ui::DialogDisplayOptions> ui;
  std::size_t nSpecies;
};

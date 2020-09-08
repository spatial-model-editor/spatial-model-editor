#pragma once
#include "simulate_options.hpp"
#include <QDialog>
#include <memory>

namespace Ui {
class DialogSimulationOptions;
}

namespace simulate {
struct IntegratorOptions;
}

class DialogSimulationOptions : public QDialog {
  Q_OBJECT

public:
  explicit DialogSimulationOptions(const simulate::Options &options,
                                   QWidget *parent = nullptr);
  ~DialogSimulationOptions();
  const simulate::Options &getOptions() const;

private:
  void setupConnections();
  void loadDuneOpts();
  void cmbDuneIntegrator_currentIndexChanged(int index);
  void txtDuneDt_editingFinished();
  void txtDuneMinDt_editingFinished();
  void txtDuneMaxDt_editingFinished();
  void txtDuneIncrease_editingFinished();
  void txtDuneDecrease_editingFinished();
  void chkDuneVTK_stateChanged();
  void resetDuneToDefaults();
  void loadPixelOpts();
  void cmbPixelIntegrator_currentIndexChanged(int index);
  void txtPixelAbsErr_editingFinished();
  void txtPixelRelErr_editingFinished();
  void txtPixelDt_editingFinished();
  void chkPixelMultithread_stateChanged();
  void spnPixelThreads_valueChanged(int value);
  void chkPixelCSE_stateChanged();
  void spnPixelOptLevel_valueChanged(int value);
  void resetPixelToDefaults();
  std::unique_ptr<Ui::DialogSimulationOptions> ui;
  simulate::Options opt;
};

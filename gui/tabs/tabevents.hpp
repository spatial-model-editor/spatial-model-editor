// TabEvents

#pragma once

#include <QWidget>
#include <memory>

namespace Ui {
class TabEvents;
}

namespace sme::model {
class Model;
} // namespace sme::model

class TabEvents : public QWidget {
  Q_OBJECT

public:
  explicit TabEvents(sme::model::Model &m, QWidget *parent = nullptr);
  ~TabEvents() override;
  void loadModelData(const QString &selection = {});

private:
  std::unique_ptr<Ui::TabEvents> ui;
  sme::model::Model &model;
  QString currentEventId{};
  QStringList variableIds{};
  int nSpecies;
  void enableWidgets(bool enable);
  void listEvents_currentRowChanged(int row);
  void btnAddEvent_clicked();
  void btnRemoveEvent_clicked();
  void txtEventName_editingFinished();
  void txtEventTime_editingFinished();
  void cmbEventVariable_activated(int index);
  void btnSetSpeciesConcentration_clicked();
  void txtExpression_mathChanged(const QString &math, bool valid,
                                 const QString &errorMessage);
};

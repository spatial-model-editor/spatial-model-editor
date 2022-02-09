// TabReactions

#pragma once

#include "sme/model_reactions.hpp"
#include <QDoubleSpinBox>
#include <QString>
#include <QWidget>
#include <memory>

namespace Ui {
class TabReactions;
}

namespace sme::model {
class Model;
}

class QLabelMouseTracker;
class QTreeWidgetItem;

class QDoubleSpinBoxNoScroll : public QDoubleSpinBox {
  Q_OBJECT

public:
  explicit QDoubleSpinBoxNoScroll(QWidget *parent = nullptr);

protected:
  void wheelEvent(QWheelEvent *event) override;
};

class TabReactions : public QWidget {
  Q_OBJECT

public:
  explicit TabReactions(sme::model::Model &m, QLabelMouseTracker *mouseTracker,
                        QWidget *parent = nullptr);
  ~TabReactions() override;
  void loadModelData(const QString &selection = {});

private:
  std::unique_ptr<Ui::TabReactions> ui;
  sme::model::Model &model;
  QLabelMouseTracker *lblGeometry;
  QString currentReacId{};
  QString invalidLocationLabel{"Invalid Location"};
  std::vector<sme::model::ReactionLocation> reactionLocations;
  QStringList invalidLocationReactionIds;

  void enableWidgets(bool enable);
  void listReactions_currentItemChanged(QTreeWidgetItem *current,
                                        QTreeWidgetItem *previous);
  void btnAddReaction_clicked();
  void btnRemoveReaction_clicked();
  void txtReactionName_editingFinished();
  void cmbReactionLocation_activated(int index);
  void listReactionParams_currentCellChanged(int currentRow, int currentColumn,
                                             int previousRow,
                                             int previousColumn);
  void listReactionParams_cellChanged(int row, int column);
  void btnAddReactionParam_clicked();
  void btnRemoveReactionParam_clicked();
  void txtReactionRate_mathChanged(const QString &math, bool valid,
                                   const QString &errorMessage);
};

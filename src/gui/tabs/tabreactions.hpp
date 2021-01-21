// TabReactions

#pragma once

#include <QString>
#include <QWidget>
#include <memory>

namespace Ui {
class TabReactions;
}

namespace sme {
namespace model {
class Model;
}
} // namespace sme

class QLabelMouseTracker;
class QTreeWidgetItem;

class TabReactions : public QWidget {
  Q_OBJECT

public:
  explicit TabReactions(sme::model::Model &m, QLabelMouseTracker *mouseTracker,
                        QWidget *parent = nullptr);
  ~TabReactions();
  void loadModelData(const QString &selection = {});

private:
  std::unique_ptr<Ui::TabReactions> ui;
  sme::model::Model &model;
  QLabelMouseTracker *lblGeometry;
  QString currentReacId;

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

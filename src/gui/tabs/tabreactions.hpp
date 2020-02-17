// TabReactions

#pragma once

#include <QString>
#include <QWidget>
#include <memory>

#include "sbml.hpp"

namespace Ui {
class TabReactions;
}

class QLabelMouseTracker;
class QTreeWidgetItem;

class TabReactions : public QWidget {
  Q_OBJECT

 public:
  explicit TabReactions(sbml::SbmlDocWrapper &doc,
                        QLabelMouseTracker *mouseTracker,
                        QWidget *parent = nullptr);
  ~TabReactions();
  void loadModelData(const QString &selection = {});

 private:
  std::unique_ptr<Ui::TabReactions> ui;
  sbml::SbmlDocWrapper &sbmlDoc;
  QLabelMouseTracker *lblGeometry;
  sbml::Reac currentReac;
  int currentReacLocIndex;

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
  void btnAddReactionParam_clicked();
  void btnRemoveReactionParam_clicked();
  void txtReactionRate_mathChanged(const QString &math, bool valid,
                                   const QString &errorMessage);
  void btnSaveReactionChanges_clicked();
};

// QTabReactions

#pragma once

#include <QString>
#include <QWidget>
#include <memory>

#include "sbml.hpp"

namespace Ui {
class QTabReactions;
}

class QLabelMouseTracker;
class QTreeWidgetItem;

class QTabReactions : public QWidget {
  Q_OBJECT

 public:
  explicit QTabReactions(sbml::SbmlDocWrapper &doc,
                         QLabelMouseTracker *mouseTracker,
                         QWidget *parent = nullptr);
  ~QTabReactions();
  void loadModelData(const QString &selection = {});

 private:
  std::unique_ptr<Ui::QTabReactions> ui;
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

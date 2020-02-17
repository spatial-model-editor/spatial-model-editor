#include <QComboBox>
#include <QFile>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTreeWidget>

#include "catch_wrapper.hpp"
#include "qlabelmousetracker.hpp"
#include "qplaintextmathedit.hpp"
#include "qt_test_utils.hpp"
#include "sbml.hpp"
#include "tabreactions.hpp"

SCENARIO("Reactions Tab", "[gui][tabs][reactions]") {
  sbml::SbmlDocWrapper sbmlDoc;
  QLabelMouseTracker mouseTracker;
  auto tab = TabReactions(sbmlDoc, &mouseTracker);
  tab.show();
  waitFor(&tab);
  ModalWidgetTimer mwt;
  // get pointers to widgets within tab
  auto *listReactions = tab.findChild<QTreeWidget *>("listReactions");
  auto *btnAddReaction = tab.findChild<QPushButton *>("btnAddReaction");
  auto *btnRemoveReaction = tab.findChild<QPushButton *>("btnRemoveReaction");
  auto *txtReactionName = tab.findChild<QLineEdit *>("txtReactionName");
  auto *cmbReactionLocation = tab.findChild<QComboBox *>("cmbReactionLocation");
  auto *listReactionSpecies =
      tab.findChild<QTreeWidget *>("listReactionSpecies");
  auto *listReactionParams =
      tab.findChild<QTableWidget *>("listReactionParams");
  auto *btnAddReactionParam =
      tab.findChild<QPushButton *>("btnAddReactionParam");
  auto *btnRemoveReactionParam =
      tab.findChild<QPushButton *>("btnRemoveReactionParam");
  auto *txtReactionRate =
      tab.findChild<QPlainTextMathEdit *>("txtReactionRate");
  auto *btnSaveReactionChanges =
      tab.findChild<QPushButton *>("btnSaveReactionChanges");
  auto *lblReactionRateStatus =
      tab.findChild<QLabel *>("lblReactionRateStatus");
  WHEN("very-simple-model loaded") {
    if (QFile f(":/models/very-simple-model.xml");
        f.open(QIODevice::ReadOnly)) {
      sbmlDoc.importSBMLString(f.readAll().toStdString());
    }
    tab.loadModelData();
    REQUIRE(listReactions->topLevelItemCount() == 5);
    REQUIRE(listReactions->topLevelItem(0)->childCount() == 0);
    REQUIRE(listReactions->topLevelItem(1)->childCount() == 0);
    REQUIRE(listReactions->topLevelItem(2)->childCount() == 1);
    REQUIRE(listReactions->topLevelItem(3)->childCount() == 2);
    REQUIRE(listReactions->topLevelItem(4)->childCount() == 2);
    REQUIRE(btnSaveReactionChanges->isEnabled() == true);
    REQUIRE(listReactions->currentItem()->text(0) == "A to B conversion");
    REQUIRE(listReactions->currentItem()->parent()->text(0) == "Nucleus");
    // edit reaction name
    txtReactionName->setFocus();
    sendKeyEvents(txtReactionName, {" ", "!", "Enter"});
    REQUIRE(listReactions->currentItem()->text(0) == "A to B conversion !");
    // change location, then cancel
    REQUIRE(cmbReactionLocation->currentIndex() == 2);
    sendKeyEvents(cmbReactionLocation, {"Down"});
    sendKeyEventsToNextQDialog({"Esc"});
    REQUIRE(cmbReactionLocation->currentIndex() == 2);
    // change location, then confirm
    REQUIRE(cmbReactionLocation->currentIndex() == 2);
    sendKeyEvents(cmbReactionLocation, {"Up"});
    sendKeyEventsToNextQDialog({"Enter"});
    REQUIRE(cmbReactionLocation->currentIndex() == 1);
    REQUIRE(listReactions->topLevelItem(1)->childCount() == 1);
    REQUIRE(listReactions->topLevelItem(2)->childCount() == 0);
    REQUIRE(listReactions->currentItem()->text(0) == "A to B conversion !");
    REQUIRE(listReactions->currentItem()->parent()->text(0) == "Cell");
    // change location back
    sendKeyEvents(cmbReactionLocation, {"Down"});
    sendKeyEventsToNextQDialog({"Enter"});
    REQUIRE(cmbReactionLocation->currentIndex() == 2);
    REQUIRE(listReactions->topLevelItem(1)->childCount() == 0);
    REQUIRE(listReactions->topLevelItem(2)->childCount() == 1);
    REQUIRE(listReactions->currentItem()->text(0) == "A to B conversion !");
    REQUIRE(listReactions->currentItem()->parent()->text(0) == "Nucleus");
    // add reaction
    mwt.addUserAction({"r", "e", "a", "c", "Q", "!"});
    mwt.start();
    sendMouseClick(btnAddReaction);
    REQUIRE(listReactions->topLevelItem(2)->childCount() == 2);
    REQUIRE(listReactions->topLevelItem(2)->child(1)->text(0) == "reacQ!");
    REQUIRE(btnRemoveReaction->isEnabled() == true);
    REQUIRE(btnRemoveReactionParam->isEnabled() == false);
    REQUIRE(listReactionParams->rowCount() == 0);
    // add reaction params
    mwt.addUserAction({"y"});
    mwt.start();
    sendMouseClick(btnAddReactionParam);
    REQUIRE(listReactionParams->rowCount() == 1);
    REQUIRE(listReactionParams->item(0, 0)->text() == "y");
    REQUIRE(btnRemoveReactionParam->isEnabled() == true);
    mwt.addUserAction({"q", "q"});
    mwt.start();
    sendMouseClick(btnAddReactionParam);
    REQUIRE(listReactionParams->rowCount() == 2);
    REQUIRE(listReactionParams->item(0, 0)->text() == "y");
    REQUIRE(listReactionParams->item(1, 0)->text() == "qq");
    REQUIRE(btnRemoveReactionParam->isEnabled() == true);
    REQUIRE(btnSaveReactionChanges->isEnabled() == false);
    // remove param, then cancel
    sendMouseClick(btnRemoveReactionParam);
    sendKeyEventsToNextQDialog({"Esc"});
    REQUIRE(listReactionParams->rowCount() == 2);
    // remove param, then confirm
    sendMouseClick(btnRemoveReactionParam);
    sendKeyEventsToNextQDialog({"Enter"});
    REQUIRE(listReactionParams->rowCount() == 1);
    REQUIRE(listReactionParams->item(0, 0)->text() == "y");
    // edit reaction rate
    txtReactionRate->setFocus();
    sendKeyEvents(txtReactionRate, {"2", "+"});
    REQUIRE(btnSaveReactionChanges->isEnabled() == false);
    REQUIRE(lblReactionRateStatus->text() == "syntax error");
    sendKeyEvents(txtReactionRate, {"y"});
    REQUIRE(btnSaveReactionChanges->isEnabled() == true);
    REQUIRE(lblReactionRateStatus->text() == "");
    // save reaction rate changes
    sendMouseClick(btnSaveReactionChanges);
    listReactions->setFocus();
    // change to membrane (i.e. not a reaction)
    sendKeyEvents(listReactions, {"Down"});
    REQUIRE(txtReactionName->isEnabled() == false);
    REQUIRE(cmbReactionLocation->isEnabled() == false);
    REQUIRE(listReactionSpecies->isEnabled() == false);
    REQUIRE(listReactionParams->isEnabled() == false);
    REQUIRE(btnAddReactionParam->isEnabled() == false);
    REQUIRE(btnRemoveReactionParam->isEnabled() == false);
    REQUIRE(txtReactionRate->isEnabled() == false);
    REQUIRE(btnSaveReactionChanges->isEnabled() == false);
    // change to reaction on a membrane
    sendKeyEvents(listReactions, {"Down"});
    REQUIRE(txtReactionName->isEnabled() == true);
    REQUIRE(cmbReactionLocation->isEnabled() == true);
    REQUIRE(listReactionSpecies->isEnabled() == true);
    REQUIRE(listReactionParams->isEnabled() == true);
    REQUIRE(btnAddReactionParam->isEnabled() == true);
    REQUIRE(btnRemoveReactionParam->isEnabled() == false);
    REQUIRE(txtReactionRate->isEnabled() == true);
    REQUIRE(btnSaveReactionChanges->isEnabled() == true);
    REQUIRE(listReactions->currentItem()->text(0) == "A uptake from outside");
    // go back to previous reaction
    sendKeyEvents(listReactions, {"Up", "Up", "Up", "Up"});
    sendKeyEvents(listReactions, {"Down", "Down"});
    REQUIRE(listReactions->currentItem()->text(0) == "reacQ!");
    REQUIRE(txtReactionRate->getMath() == "2 + y");
    // remove reaction, then cancel
    sendMouseClick(btnRemoveReaction);
    sendKeyEventsToNextQDialog({"Esc"});
    REQUIRE(listReactions->topLevelItem(2)->childCount() == 2);
    REQUIRE(listReactions->currentItem()->text(0) == "reacQ!");
    // remove reaction, then confirm
    sendMouseClick(btnRemoveReaction);
    sendKeyEventsToNextQDialog({"Enter"});
    REQUIRE(listReactions->topLevelItem(2)->childCount() == 1);
    REQUIRE(listReactions->currentItem()->text(0) == "A to B conversion !");
    // remove reaction, then confirm
    sendMouseClick(btnRemoveReaction);
    sendKeyEventsToNextQDialog({"Enter"});
    REQUIRE(listReactions->topLevelItem(2)->childCount() == 0);
    REQUIRE(listReactions->currentItem()->text(0) == "A uptake from outside");
  }
}

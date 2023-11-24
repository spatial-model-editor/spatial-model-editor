#include "catch_wrapper.hpp"
#include "model_test_utils.hpp"
#include "qlabelmousetracker.hpp"
#include "qplaintextmathedit.hpp"
#include "qt_test_utils.hpp"
#include "sme/model.hpp"
#include "tabreactions.hpp"
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTreeWidget>

using namespace sme::test;
using Catch::Matchers::ContainsSubstring;

TEST_CASE("TabReactions", "[gui/tabs/reactions][gui/tabs][gui][reactions]") {
  sme::model::Model model;
  QLabelMouseTracker mouseTracker;
  TabReactions tab(model, &mouseTracker);
  tab.show();
  waitFor(&tab);
  ModalWidgetTimer mwt;
  // get pointers to widgets within tab
  auto *listReactions{tab.findChild<QTreeWidget *>("listReactions")};
  REQUIRE(listReactions != nullptr);
  auto *btnAddReaction{tab.findChild<QPushButton *>("btnAddReaction")};
  REQUIRE(btnAddReaction != nullptr);
  auto *btnRemoveReaction{tab.findChild<QPushButton *>("btnRemoveReaction")};
  REQUIRE(btnRemoveReaction != nullptr);
  auto *txtReactionName{tab.findChild<QLineEdit *>("txtReactionName")};
  REQUIRE(txtReactionName != nullptr);
  auto *lblReactionScheme{tab.findChild<QLabel *>("lblReactionScheme")};
  REQUIRE(lblReactionScheme != nullptr);
  auto *cmbReactionLocation{tab.findChild<QComboBox *>("cmbReactionLocation")};
  REQUIRE(cmbReactionLocation != nullptr);
  auto *listReactionSpecies{
      tab.findChild<QTreeWidget *>("listReactionSpecies")};
  REQUIRE(listReactionSpecies != nullptr);
  auto *listReactionParams{tab.findChild<QTableWidget *>("listReactionParams")};
  REQUIRE(listReactionParams != nullptr);
  auto *btnAddReactionParam{
      tab.findChild<QPushButton *>("btnAddReactionParam")};
  REQUIRE(btnAddReactionParam != nullptr);
  auto *btnRemoveReactionParam{
      tab.findChild<QPushButton *>("btnRemoveReactionParam")};
  REQUIRE(btnRemoveReactionParam != nullptr);
  auto *txtReactionRate{tab.findChild<QPlainTextMathEdit *>("txtReactionRate")};
  REQUIRE(txtReactionRate != nullptr);
  auto *lblReactionRateStatus{tab.findChild<QLabel *>("lblReactionRateStatus")};
  REQUIRE(lblReactionRateStatus != nullptr);
  SECTION("very-simple-model loaded") {
    model = getExampleModel(Mod::VerySimpleModel);
    tab.loadModelData();
    REQUIRE(listReactions->topLevelItemCount() == 5);
    REQUIRE(listReactions->topLevelItem(0)->childCount() == 0);
    REQUIRE(listReactions->topLevelItem(1)->childCount() == 0);
    REQUIRE(listReactions->topLevelItem(2)->childCount() == 1);
    REQUIRE(listReactions->topLevelItem(3)->childCount() == 2);
    REQUIRE(listReactions->topLevelItem(4)->childCount() == 2);
    REQUIRE(listReactions->currentItem()->text(0) == "A to B conversion");
    REQUIRE(listReactions->currentItem()->parent()->text(0) == "Nucleus");
    REQUIRE(lblReactionScheme->text() == "A_nucl -> B_nucl");
    // edit reaction name
    txtReactionName->setFocus();
    sendKeyEvents(txtReactionName, {" ", "!", "Enter"});
    REQUIRE(listReactions->currentItem()->text(0) == "A to B conversion !");
    REQUIRE(model.getReactions().getName("A_B_conversion") ==
            "A to B conversion !");
    // change location
    REQUIRE(cmbReactionLocation->currentIndex() == 2);
    cmbReactionLocation->setFocus();
    sendKeyEvents(cmbReactionLocation, {"Down"});
    REQUIRE(cmbReactionLocation->currentIndex() == 3);
    REQUIRE(listReactions->topLevelItem(1)->childCount() == 0);
    REQUIRE(listReactions->topLevelItem(2)->childCount() == 0);
    REQUIRE(listReactions->topLevelItem(3)->childCount() == 3);
    // same reaction selected as previously
    REQUIRE(listReactions->currentItem()->text(0) == "A to B conversion !");
    REQUIRE(listReactions->currentItem()->parent()->text(0) ==
            "Outside <-> Cell");
    REQUIRE(model.getReactions().getLocation("A_B_conversion") ==
            "c1_c2_membrane");
    // change location back
    sendKeyEvents(cmbReactionLocation, {"Up"});
    REQUIRE(cmbReactionLocation->currentIndex() == 2);
    REQUIRE(listReactions->topLevelItem(1)->childCount() == 0);
    REQUIRE(listReactions->topLevelItem(2)->childCount() == 1);
    REQUIRE(listReactions->topLevelItem(3)->childCount() == 2);
    // same reaction still selected
    REQUIRE(listReactions->currentItem()->text(0) == "A to B conversion !");
    REQUIRE(listReactions->currentItem()->parent()->text(0) == "Nucleus");
    REQUIRE(model.getReactions().getLocation("A_B_conversion") == "c3");
    // add reaction
    mwt.addUserAction({"r", "e", "a", "c", "Q", "!"});
    mwt.start();
    sendMouseClick(btnAddReaction);
    REQUIRE(listReactions->topLevelItem(2)->childCount() == 2);
    REQUIRE(listReactions->topLevelItem(2)->child(1)->text(0) == "reacQ!");
    REQUIRE(btnRemoveReaction->isEnabled() == true);
    REQUIRE(btnRemoveReactionParam->isEnabled() == false);
    REQUIRE(listReactionParams->rowCount() == 0);
    REQUIRE(model.getReactions().getIds("c3").size() == 2);
    REQUIRE(model.getReactions().getName("reacQ") == "reacQ!");
    REQUIRE(lblReactionScheme->text() == "");
    // add & edit reaction params
    mwt.addUserAction({"y"});
    mwt.start();
    sendMouseClick(btnAddReactionParam);
    REQUIRE(listReactionParams->rowCount() == 1);
    REQUIRE(listReactionParams->item(0, 0)->text() == "y");
    REQUIRE(btnRemoveReactionParam->isEnabled() == true);
    REQUIRE(model.getReactions().getParameterIds("reacQ").size() == 1);
    REQUIRE(model.getReactions().getParameterIds("reacQ")[0] == "y_");
    REQUIRE(model.getReactions().getParameterName("reacQ", "y_") == "y");
    REQUIRE(model.getReactions().getParameterValue("reacQ", "y_") ==
            dbl_approx(0));
    // edit value
    listReactionParams->item(0, 1)->setText("3.14159");
    REQUIRE(model.getReactions().getParameterValue("reacQ", "y_") ==
            dbl_approx(3.14159));
    // edit name
    listReactionParams->item(0, 0)->setText("y !!");
    REQUIRE(model.getReactions().getParameterIds("reacQ").size() == 1);
    REQUIRE(model.getReactions().getParameterIds("reacQ")[0] == "y_");
    REQUIRE(model.getReactions().getParameterName("reacQ", "y_") == "y !!");
    REQUIRE(model.getReactions().getParameterValue("reacQ", "y_") ==
            dbl_approx(3.14159));
    mwt.addUserAction({"q", "q"});
    mwt.start();
    sendMouseClick(btnAddReactionParam);
    REQUIRE(listReactionParams->rowCount() == 2);
    REQUIRE(listReactionParams->item(0, 0)->text() == "y !!");
    REQUIRE(listReactionParams->item(1, 0)->text() == "qq");
    REQUIRE(btnRemoveReactionParam->isEnabled() == true);
    REQUIRE(model.getReactions().getParameterIds("reacQ").size() == 2);
    REQUIRE(model.getReactions().getParameterIds("reacQ")[0] == "y_");
    REQUIRE(model.getReactions().getParameterIds("reacQ")[1] == "qq");
    REQUIRE(model.getReactions().getParameterName("reacQ", "y_") == "y !!");
    REQUIRE(model.getReactions().getParameterValue("reacQ", "y_") ==
            dbl_approx(3.14159));
    REQUIRE(model.getReactions().getParameterName("reacQ", "qq") == "qq");
    REQUIRE(model.getReactions().getParameterValue("reacQ", "qq") ==
            dbl_approx(0));
    // remove param, then cancel
    mwt.addUserAction({"Esc"});
    mwt.start();
    sendMouseClick(btnRemoveReactionParam);
    REQUIRE(listReactionParams->rowCount() == 2);
    REQUIRE(model.getReactions().getParameterIds("reacQ").size() == 2);
    // remove param, then confirm
    mwt.addUserAction({"Enter"});
    mwt.start();
    sendMouseClick(btnRemoveReactionParam);
    REQUIRE(listReactionParams->rowCount() == 1);
    REQUIRE(listReactionParams->item(0, 0)->text() == "y !!");
    REQUIRE(model.getReactions().getParameterIds("reacQ").size() == 1);
    // change name back to "y"
    listReactionParams->item(0, 0)->setText("y");
    // edit reaction rate
    txtReactionRate->setFocus();
    sendKeyEvents(txtReactionRate, {"Backspace", "Delete", "2", "+"});
    REQUIRE_THAT(lblReactionRateStatus->text().toStdString(),
                 ContainsSubstring("syntax error"));
    sendKeyEvents(txtReactionRate, {"y"});
    REQUIRE(model.getReactions().getParameterName("reacQ", "y_") == "y");
    REQUIRE(lblReactionRateStatus->text() == "");
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
    // change to reaction on a membrane
    sendKeyEvents(listReactions, {"Down"});
    REQUIRE(txtReactionName->isEnabled() == true);
    REQUIRE(cmbReactionLocation->isEnabled() == true);
    REQUIRE(listReactionSpecies->isEnabled() == true);
    REQUIRE(listReactionParams->isEnabled() == true);
    REQUIRE(btnAddReactionParam->isEnabled() == true);
    REQUIRE(btnRemoveReactionParam->isEnabled() == false);
    REQUIRE(txtReactionRate->isEnabled() == true);
    REQUIRE(listReactions->currentItem()->text(0) == "A uptake from outside");
    // go back to previous reaction
    sendKeyEvents(listReactions, {"Up", "Up", "Up", "Up"});
    sendKeyEvents(listReactions, {"Down", "Down"});
    REQUIRE(listReactions->currentItem()->text(0) == "reacQ!");
    REQUIRE(txtReactionRate->getMath() == "2 + y");
    // remove reaction, then cancel
    mwt.addUserAction({"Esc"});
    mwt.start();
    sendMouseClick(btnRemoveReaction);
    REQUIRE(listReactions->topLevelItem(2)->childCount() == 2);
    REQUIRE(listReactions->currentItem()->text(0) == "reacQ!");
    // remove reaction, then confirm
    mwt.addUserAction({"Enter"});
    mwt.start();
    sendMouseClick(btnRemoveReaction);
    REQUIRE(listReactions->topLevelItem(2)->childCount() == 1);
    REQUIRE(listReactions->currentItem()->text(0) == "A to B conversion !");
    // remove reaction, then confirm
    mwt.addUserAction({"Enter"});
    mwt.start();
    sendMouseClick(btnRemoveReaction);
    REQUIRE(listReactions->topLevelItem(2)->childCount() == 0);
    REQUIRE(listReactions->currentItem()->text(0) == "A uptake from outside");
  }
  SECTION("model with invalid reaction locations loaded") {
    model = getTestModel("very-simple-model-invalid-reaction-location");
    tab.loadModelData();
    REQUIRE(listReactions->topLevelItemCount() == 6);
    REQUIRE(listReactions->topLevelItem(0)->childCount() == 0);
    REQUIRE(listReactions->topLevelItem(1)->childCount() == 0);
    REQUIRE(listReactions->topLevelItem(2)->childCount() == 1);
    REQUIRE(listReactions->topLevelItem(3)->childCount() == 2);
    REQUIRE(listReactions->topLevelItem(4)->childCount() == 0);
    REQUIRE(listReactions->topLevelItem(5)->childCount() == 2);
    REQUIRE(listReactions->topLevelItem(5)->text(0) == "Invalid Location");
    REQUIRE(listReactions->currentItem()->text(0) == "A to B conversion");
    REQUIRE(listReactions->currentItem()->parent()->text(0) == "Nucleus");
    REQUIRE(lblReactionScheme->text() == "A_nucl -> B_nucl");
    listReactions->setFocus();
    // change to invalid reaction section
    sendKeyEvents(listReactions, {"Down", "Down", "Down", "Down", "Down"});
    REQUIRE(listReactions->currentItem()->text(0) == "Invalid Location");
    REQUIRE(txtReactionName->isEnabled() == false);
    REQUIRE(cmbReactionLocation->isEnabled() == false);
    REQUIRE(listReactionSpecies->isEnabled() == false);
    REQUIRE(listReactionParams->isEnabled() == false);
    REQUIRE(btnAddReactionParam->isEnabled() == false);
    REQUIRE(btnRemoveReactionParam->isEnabled() == false);
    REQUIRE(txtReactionRate->isEnabled() == false);
    REQUIRE(btnRemoveReaction->isEnabled() == false);
    // first invalid location reaction: only remove & location enabled
    sendKeyEvents(listReactions, {"Down"});
    REQUIRE(listReactions->currentItem()->text(0) == "A internal transport");
    REQUIRE(txtReactionName->isEnabled() == false);
    REQUIRE(txtReactionName->text() == "A internal transport");
    REQUIRE(cmbReactionLocation->isEnabled() == true);
    REQUIRE(listReactionSpecies->isEnabled() == false);
    REQUIRE(listReactionParams->isEnabled() == false);
    REQUIRE(btnAddReactionParam->isEnabled() == false);
    REQUIRE(btnRemoveReactionParam->isEnabled() == false);
    REQUIRE(txtReactionRate->isEnabled() == false);
    REQUIRE(btnRemoveReaction->isEnabled() == true);
    // set a valid location for the reaction
    cmbReactionLocation->setFocus();
    REQUIRE(cmbReactionLocation->currentIndex() == -1);
    sendKeyEvents(cmbReactionLocation, {"Down"});
    REQUIRE(cmbReactionLocation->currentIndex() == 0);
    REQUIRE(listReactions->topLevelItem(0)->childCount() == 1);
    REQUIRE(listReactions->topLevelItem(1)->childCount() == 0);
    REQUIRE(listReactions->topLevelItem(2)->childCount() == 1);
    REQUIRE(listReactions->topLevelItem(3)->childCount() == 2);
    REQUIRE(listReactions->topLevelItem(4)->childCount() == 0);
    REQUIRE(listReactions->topLevelItem(5)->childCount() == 1);
    // same reaction selected as previously
    REQUIRE(listReactions->currentItem()->text(0) == "A internal transport");
    REQUIRE(listReactions->currentItem()->parent()->text(0) == "Outside");
    REQUIRE(model.getReactions().getLocation("A_transport") == "c1");
    // select remaining invalid location reaction
    sendKeyEvents(listReactions, {"Down", "Down", "Down", "Down", "Down",
                                  "Down", "Down", "Down", "Down"});
    REQUIRE(listReactions->currentItem()->text(0) == "B internal transport");
    REQUIRE(txtReactionName->isEnabled() == false);
    REQUIRE(txtReactionName->text() == "B internal transport");
    REQUIRE(cmbReactionLocation->isEnabled() == true);
    REQUIRE(listReactionSpecies->isEnabled() == false);
    REQUIRE(listReactionParams->isEnabled() == false);
    REQUIRE(btnAddReactionParam->isEnabled() == false);
    REQUIRE(btnRemoveReactionParam->isEnabled() == false);
    REQUIRE(txtReactionRate->isEnabled() == false);
    REQUIRE(btnRemoveReaction->isEnabled() == true);
    // remove reaction, then cancel
    mwt.addUserAction({"Esc"});
    mwt.start();
    sendMouseClick(btnRemoveReaction);
    REQUIRE(listReactions->topLevelItem(5)->childCount() == 1);
    REQUIRE(listReactions->currentItem()->text(0) == "B internal transport");
    // remove reaction, then confirm
    REQUIRE(listReactions->topLevelItemCount() == 6);
    mwt.addUserAction({"Enter"});
    mwt.start();
    sendMouseClick(btnRemoveReaction);
    // "Invalid Location" section is completely removed once there are no
    // reactions with invalid locations to display
    REQUIRE(listReactions->topLevelItemCount() == 5);
  }
}

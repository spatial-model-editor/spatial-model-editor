#include "catch_wrapper.hpp"
#include "model.hpp"
#include "qplaintextmathedit.hpp"
#include "qt_test_utils.hpp"
#include "tabevents.hpp"
#include "model_test_utils.hpp"
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>

using namespace sme::test;

SCENARIO("Events Tab", "[gui/tabs/events][gui/tabs][gui][events]") {
  sme::model::Model model;
  auto tab = TabEvents(model);
  // get pointers to widgets within tab
  auto *listEvents{tab.findChild<QListWidget *>("listEvents")};
  REQUIRE(listEvents != nullptr);
  auto *btnAddEvent{tab.findChild<QPushButton *>("btnAddEvent")};
  REQUIRE(btnAddEvent != nullptr);
  auto *btnRemoveEvent{tab.findChild<QPushButton *>("btnRemoveEvent")};
  REQUIRE(btnRemoveEvent != nullptr);
  auto *txtEventName{tab.findChild<QLineEdit *>("txtEventName")};
  REQUIRE(txtEventName != nullptr);
  auto *txtEventTime{tab.findChild<QLineEdit *>("txtEventTime")};
  REQUIRE(txtEventTime != nullptr);
  auto *cmbEventVariable{tab.findChild<QComboBox *>("cmbEventVariable")};
  REQUIRE(cmbEventVariable != nullptr);
  auto *txtExpression{tab.findChild<QPlainTextMathEdit *>("txtExpression")};
  REQUIRE(txtExpression != nullptr);
  auto *btnSetSpeciesConcentration{tab.findChild<QPushButton *>("btnSetSpeciesConcentration")};
  REQUIRE(btnSetSpeciesConcentration != nullptr);
  auto *lblSpeciesExpression{tab.findChild<QLabel *>("lblSpeciesExpression")};
  REQUIRE(lblSpeciesExpression != nullptr);
  auto *lblExpressionStatus{tab.findChild<QLabel *>("lblExpressionStatus")};
  REQUIRE(lblExpressionStatus != nullptr);
  REQUIRE(txtExpression != nullptr);
  ModalWidgetTimer mwt;
  GIVEN("ABtoC model") {
    model = getExampleModel(Mod::ABtoC);
    const auto &events{model.getEvents()};
    REQUIRE(model.getSpecies().getIds("comp").size() == 3);
    model.getSpecies().remove("A");
    model.getSpecies().remove("B");
    model.getSpecies().remove("C");
    REQUIRE(model.getSpecies().getIds("comp").isEmpty());
    tab.loadModelData();
    REQUIRE(events.getIds().size() == 0);
    REQUIRE(listEvents->count() == 0);
    REQUIRE(btnAddEvent->isEnabled() == true);
    REQUIRE(btnRemoveEvent->isEnabled() == false);
    REQUIRE(txtEventName->isEnabled() == false);
    REQUIRE(txtEventTime->isEnabled() == false);
    REQUIRE(cmbEventVariable->isEnabled() == false);
    REQUIRE(txtExpression->isEnabled() == false);
    REQUIRE(btnSetSpeciesConcentration->isEnabled() == false);
    REQUIRE(lblExpressionStatus->text() == "");
    // try to add Event to a model with no parameters or species
    mwt.start();
    sendMouseClick(btnAddEvent);
    REQUIRE(mwt.getResult() == "To add events, the model must contain species or parameters.");
  }
  GIVEN("very simple model") {
    model = getExampleModel(Mod::VerySimpleModel);
    const auto &events{model.getEvents()};
    tab.loadModelData();
    tab.show();
    REQUIRE(events.getIds().size() == 0);
    REQUIRE(listEvents->count() == 0);
    REQUIRE(btnAddEvent->isEnabled() == true);
    REQUIRE(btnRemoveEvent->isEnabled() == false);
    REQUIRE(txtEventName->isEnabled() == false);
    REQUIRE(txtEventTime->isEnabled() == false);
    REQUIRE(cmbEventVariable->isEnabled() == false);
    REQUIRE(txtExpression->isEnabled() == false);
    REQUIRE(lblExpressionStatus->text() == "");
    // add Event
    mwt.addUserAction({"p", " ", "!"});
    mwt.start();
    sendMouseClick(btnAddEvent);
    REQUIRE(listEvents->count() == 1);
    REQUIRE(listEvents->currentItem()->text() == "p !");
    REQUIRE(txtEventName->text() == "p !");
    REQUIRE(txtEventTime->isEnabled() == true);
    REQUIRE(txtEventTime->text() == "0");
    REQUIRE(cmbEventVariable->isEnabled() == true);
    REQUIRE(cmbEventVariable->count() == 7);
    REQUIRE(cmbEventVariable->currentIndex() == 0);
    REQUIRE(btnAddEvent->isEnabled() == true);
    REQUIRE(btnRemoveEvent->isEnabled() == true);
    REQUIRE(txtExpression->isEnabled() == false);
    REQUIRE(txtExpression->isVisible() == false);
    REQUIRE(btnSetSpeciesConcentration->isEnabled() == true);
    REQUIRE(events.getIds().size() == 1);
    REQUIRE(events.getIds()[0] == "p_");
    REQUIRE(events.getNames()[0] == "p !");
    REQUIRE(events.getVariable("p_") == "A_c1");
    // edit name
    txtEventName->setFocus();
    sendKeyEvents(txtEventName, {"z", "Enter"});
    REQUIRE(txtEventName->text() == "p !z");
    REQUIRE(listEvents->currentItem()->text() == "p !z");
    REQUIRE(events.getIds()[0] == "p_");
    REQUIRE(events.getNames()[0] == "p !z");
    REQUIRE(events.getName("p_") == "p !z");
    REQUIRE(events.getExpression("p_") == "0");
    // edit concentration: 0 -> x
    mwt.addUserAction({"Del", "Backspace", "x"});
    mwt.start();
    sendMouseClick(btnSetSpeciesConcentration);
    REQUIRE(events.getExpression("p_") == "x");
    // change variable to a parameter
    cmbEventVariable->setFocus();
    sendKeyEvents(cmbEventVariable, {"Down", "Down", "Down", "Down", "Down", "Down", "Down"});
    REQUIRE(events.getVariable("p_") == "param");
    // edit time
    REQUIRE(events.getTime("p_") == dbl_approx(0));
    txtEventTime->setFocus();
    sendKeyEvents(txtEventTime,
                  {"Backspace", "Del", "2", "4", ".", "9", "Enter"});
    REQUIRE(txtEventTime->text() == "24.9");
    REQUIRE(events.getTime("p_") == dbl_approx(24.9));
    // expression -> ""
    txtExpression->setFocus();
    sendKeyEvents(txtExpression, {"Delete", "Backspace", "Backspace"});
    REQUIRE(txtExpression->toPlainText() == "");
    REQUIRE(lblExpressionStatus->text() == "Empty expression");
    // invalid expression so model expression unchanged
    REQUIRE(events.getExpression("p_") == "x");
    // expression -> "2"
    sendKeyEvents(txtExpression, {"2"});
    REQUIRE(txtExpression->toPlainText() == "2");
    REQUIRE(lblExpressionStatus->text() == "");
    REQUIRE(events.getExpression("p_") == "2");
    // expression -> "2+cos(1)"
    sendKeyEvents(txtExpression, {"+", "c", "o", "s", "(", "1", ")"});
    REQUIRE(txtExpression->toPlainText() == "2+cos(1)");
    REQUIRE(lblExpressionStatus->text() == "");
    REQUIRE(events.getExpression("p_") == "2 + cos(1)");
    // add Event with same name
    mwt.addUserAction({"p", " ", "!", "z"});
    mwt.start();
    sendMouseClick(btnAddEvent);
    REQUIRE(listEvents->count() == 2);
    REQUIRE(events.getIds().size() == 2);
    // name changed to make it unique
    REQUIRE(listEvents->currentItem()->text() == "p !z_");
    // remove Event & cancel
    sendMouseClick(btnRemoveEvent);
    sendKeyEventsToNextQDialog({"Esc"});
    REQUIRE(listEvents->count() == 2);
    REQUIRE(events.getIds().size() == 2);
    REQUIRE(listEvents->currentItem()->text() == "p !z_");
    // remove Event & confirm
    sendMouseClick(btnRemoveEvent);
    sendKeyEventsToNextQDialog({"Enter"});
    REQUIRE(listEvents->count() == 1);
    REQUIRE(events.getIds().size() == 1);
    REQUIRE(listEvents->currentItem()->text() == "p !z");
    // remove Event & confirm
    sendMouseClick(btnRemoveEvent);
    sendKeyEventsToNextQDialog({"Enter"});
    REQUIRE(listEvents->count() == 0);
    REQUIRE(events.getIds().size() == 0);
  }
  GIVEN("brusselator model") {
    model = getExampleModel(Mod::Brusselator);
    const auto &events{model.getEvents()};
    tab.loadModelData();
    REQUIRE(listEvents->count() == 2);
    REQUIRE(listEvents->currentItem()->text() == "increase k2");
    REQUIRE(txtEventName->text() == "increase k2");
    REQUIRE(txtEventTime->isEnabled() == true);
    REQUIRE(txtEventTime->text() == "25");
    REQUIRE(cmbEventVariable->isEnabled() == true);
    REQUIRE(cmbEventVariable->count() == 8);
    REQUIRE(cmbEventVariable->currentIndex() == 6);
    REQUIRE(btnAddEvent->isEnabled() == true);
    REQUIRE(btnRemoveEvent->isEnabled() == true);
    REQUIRE(events.getIds().size() == 2);
    REQUIRE(events.getIds()[0] == "double_k2");
    REQUIRE(events.getIds()[1] == "reset_k2");
    REQUIRE(events.getNames()[0] == "increase k2");
    REQUIRE(events.getNames()[1] == "decrease k2");
    // change event param
    REQUIRE(events.getVariable("double_k2") == "k2");
    REQUIRE(cmbEventVariable->currentIndex() == 6);
    cmbEventVariable->setFocus();
    sendKeyEvents(cmbEventVariable, {"Down", "Enter"});
    REQUIRE(events.getVariable("double_k2") == "k3");
    REQUIRE(cmbEventVariable->currentIndex() == 7);
    cmbEventVariable->setFocus();
    sendKeyEvents(cmbEventVariable, {"Up", "Enter"});
    REQUIRE(events.getVariable("double_k2") == "k2");
    REQUIRE(cmbEventVariable->currentIndex() == 6);
  }
}

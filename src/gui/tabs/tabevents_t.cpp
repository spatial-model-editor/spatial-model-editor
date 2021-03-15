#include "catch_wrapper.hpp"
#include "model.hpp"
#include "qplaintextmathedit.hpp"
#include "qt_test_utils.hpp"
#include "tabevents.hpp"
#include <QComboBox>
#include <QDebug>
#include <QDialog>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>

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
  auto *cmbEventParam{tab.findChild<QComboBox *>("cmbEventParam")};
  REQUIRE(cmbEventParam != nullptr);
  auto *txtExpression{tab.findChild<QPlainTextMathEdit *>("txtExpression")};
  REQUIRE(txtExpression != nullptr);
  auto *lblExpressionStatus{tab.findChild<QLabel *>("lblExpressionStatus")};
  REQUIRE(lblExpressionStatus != nullptr);
  const auto &events = model.getEvents();
  REQUIRE(txtExpression != nullptr);
  ModalWidgetTimer mwt;
  GIVEN("ABtoC model") {
    QFile f(":/models/ABtoC.xml");
    f.open(QIODevice::ReadOnly);
    model.importSBMLString(f.readAll().toStdString());
    tab.loadModelData();
    REQUIRE(events.getIds().size() == 0);
    REQUIRE(listEvents->count() == 0);
    REQUIRE(btnAddEvent->isEnabled() == true);
    REQUIRE(btnRemoveEvent->isEnabled() == false);
    REQUIRE(txtEventName->isEnabled() == false);
    REQUIRE(txtEventTime->isEnabled() == false);
    REQUIRE(cmbEventParam->isEnabled() == false);
    REQUIRE(txtExpression->isEnabled() == false);
    REQUIRE(lblExpressionStatus->text() == "");
    // try to add Event to a model with no parameters
    mwt.start();
    sendMouseClick(btnAddEvent);
    REQUIRE(mwt.getResult() == "To add events, the model must contain parameters.");
  }
  GIVEN("very simple model") {
    QFile f(":/models/very-simple-model.xml");
    f.open(QIODevice::ReadOnly);
    model.importSBMLString(f.readAll().toStdString());
    tab.loadModelData();
    REQUIRE(events.getIds().size() == 0);
    REQUIRE(listEvents->count() == 0);
    REQUIRE(btnAddEvent->isEnabled() == true);
    REQUIRE(btnRemoveEvent->isEnabled() == false);
    REQUIRE(txtEventName->isEnabled() == false);
    REQUIRE(txtEventTime->isEnabled() == false);
    REQUIRE(cmbEventParam->isEnabled() == false);
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
    REQUIRE(cmbEventParam->isEnabled() == true);
    REQUIRE(cmbEventParam->count() == 1);
    REQUIRE(cmbEventParam->currentIndex() == 0);
    REQUIRE(btnAddEvent->isEnabled() == true);
    REQUIRE(btnRemoveEvent->isEnabled() == true);
    REQUIRE(events.getIds().size() == 1);
    REQUIRE(events.getIds()[0] == "p_");
    REQUIRE(events.getNames()[0] == "p !");
    // edit name
    txtEventName->setFocus();
    sendKeyEvents(txtEventName, {"z", "Enter"});
    REQUIRE(txtEventName->text() == "p !z");
    REQUIRE(listEvents->currentItem()->text() == "p !z");
    REQUIRE(events.getIds()[0] == "p_");
    REQUIRE(events.getNames()[0] == "p !z");
    REQUIRE(events.getName("p_") == "p !z");
    REQUIRE(txtExpression->toPlainText() == "0");
    // edit time
    REQUIRE(events.getTime("p_") == dbl_approx(0));
    txtEventTime->setFocus();
    sendKeyEvents(txtEventTime,
                  {"Backspace", "Del", "2", "4", ".", "9", "Enter"});
    REQUIRE(txtEventTime->text() == "24.9");
    REQUIRE(events.getTime("p_") == dbl_approx(24.9));
    // expression -> ""
    txtExpression->setFocus();
    sendKeyEvents(txtExpression, {"Delete", "Backspace"});
    REQUIRE(txtExpression->toPlainText() == "");
    REQUIRE(lblExpressionStatus->text() == "Empty expression");
    // invalid expression so model expression unchanged
    REQUIRE(events.getExpression("p_") == "0");
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
    QFile f(":/models/brusselator-model.xml");
    f.open(QIODevice::ReadOnly);
    model.importSBMLString(f.readAll().toStdString());
    tab.loadModelData();
    REQUIRE(listEvents->count() == 2);
    REQUIRE(listEvents->currentItem()->text() == "increase k2");
    REQUIRE(txtEventName->text() == "increase k2");
    REQUIRE(txtEventTime->isEnabled() == true);
    REQUIRE(txtEventTime->text() == "25");
    REQUIRE(cmbEventParam->isEnabled() == true);
    REQUIRE(cmbEventParam->count() == 2);
    REQUIRE(cmbEventParam->currentIndex() == 0);
    REQUIRE(btnAddEvent->isEnabled() == true);
    REQUIRE(btnRemoveEvent->isEnabled() == true);
    REQUIRE(events.getIds().size() == 2);
    REQUIRE(events.getIds()[0] == "double_k2");
    REQUIRE(events.getIds()[1] == "reset_k2");
    REQUIRE(events.getNames()[0] == "increase k2");
    REQUIRE(events.getNames()[1] == "decrease k2");
    // change event param
    REQUIRE(events.getVariable("double_k2") == "k2");
    REQUIRE(cmbEventParam->currentIndex() == 0);
    cmbEventParam->setFocus();
    sendKeyEvents(cmbEventParam, {"Down", "Enter"});
    REQUIRE(events.getVariable("double_k2") == "k3");
    REQUIRE(cmbEventParam->currentIndex() == 1);
    cmbEventParam->setFocus();
    sendKeyEvents(cmbEventParam, {"Up", "Enter"});
    REQUIRE(events.getVariable("double_k2") == "k2");
    REQUIRE(cmbEventParam->currentIndex() == 0);
  }
}

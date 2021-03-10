#include "catch_wrapper.hpp"
#include "model.hpp"
#include "qplaintextmathedit.hpp"
#include "qt_test_utils.hpp"
#include "tabevents.hpp"
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
  auto *listEvents = tab.findChild<QListWidget *>("listEvents");
  auto *btnAddEvent = tab.findChild<QPushButton *>("btnAddEvent");
  auto *btnRemoveEvent = tab.findChild<QPushButton *>("btnRemoveEvent");
  auto *txtEventName = tab.findChild<QLineEdit *>("txtEventName");
  auto *txtExpression = tab.findChild<QPlainTextMathEdit *>("txtExpression");
  auto *lblExpressionStatus = tab.findChild<QLabel *>("lblExpressionStatus");
  const auto &events = model.getEvents();
  REQUIRE(txtExpression != nullptr);
  ModalWidgetTimer mwt;
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
    REQUIRE(txtExpression->isEnabled() == false);
    REQUIRE(lblExpressionStatus->text() == "");
    // add Event
    mwt.addUserAction({"p", " ", "!"});
    mwt.start();
    sendMouseClick(btnAddEvent);
    REQUIRE(listEvents->count() == 1);
    REQUIRE(listEvents->currentItem()->text() == "p !");
    REQUIRE(txtEventName->text() == "p !");
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
    //    REQUIRE(txtExpression->toPlainText() == "0");
    //    // expression -> ""
    //    txtExpression->setFocus();
    //    sendKeyEvents(txtExpression, {"Delete", "Backspace"});
    //    REQUIRE(txtExpression->toPlainText() == "");
    //    REQUIRE(lblExpressionStatus->text() == "Empty expression");
    //    // invalid expression so model expression unchanged
    //    REQUIRE(events.getExpression("p_") == "0");
    //    // expression -> "2"
    //    sendKeyEvents(txtExpression, {"2"});
    //    REQUIRE(txtExpression->toPlainText() == "2");
    //    REQUIRE(lblExpressionStatus->text() == "");
    //    REQUIRE(events.getExpression("p_") == "2");
    //    // expression -> "2+cos(1)"
    //    sendKeyEvents(txtExpression, {"+", "c", "o", "s", "(", "1", ")"});
    //    REQUIRE(txtExpression->toPlainText() == "2+cos(1)");
    //    REQUIRE(lblExpressionStatus->text() == "");
    //    REQUIRE(events.getExpression("p_") == "2 + cos(1)");
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
}

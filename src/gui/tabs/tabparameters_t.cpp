#include <QDebug>
#include <QDialog>
#include <QFile>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>

#include "catch_wrapper.hpp"
#include "model.hpp"
#include "qplaintextmathedit.hpp"
#include "qt_test_utils.hpp"
#include "tabparameters.hpp"

SCENARIO("Parameters Tab", "[gui/tabs/parameters][gui/tabs][gui][parameters]") {
  model::Model model;
  auto tab = TabParameters(model);
  // get pointers to widgets within tab
  auto *listParameters = tab.findChild<QListWidget *>("listParameters");
  auto *btnAddParameter = tab.findChild<QPushButton *>("btnAddParameter");
  auto *btnRemoveParameter = tab.findChild<QPushButton *>("btnRemoveParameter");
  auto *txtParameterName = tab.findChild<QLineEdit *>("txtParameterName");
  auto *txtExpression = tab.findChild<QPlainTextMathEdit *>("txtExpression");
  const auto &params = model.getParameters();
  REQUIRE(txtExpression != nullptr);
  ModalWidgetTimer mwt;
  GIVEN("very simple model") {
    if (QFile f(":/models/very-simple-model.xml");
        f.open(QIODevice::ReadOnly)) {
      model.importSBMLString(f.readAll().toStdString());
    }
    tab.loadModelData();
    REQUIRE(listParameters->count() == 1);
    REQUIRE(btnAddParameter->isEnabled() == true);
    REQUIRE(btnRemoveParameter->isEnabled() == true);
    REQUIRE(params.getIds().size() == 1);
    REQUIRE(params.getNames().size() == 1);
    // remove parameter & confirm
    sendMouseClick(btnRemoveParameter);
    sendKeyEventsToNextQDialog({"Enter"});
    REQUIRE(listParameters->count() == 0);
    REQUIRE(btnAddParameter->isEnabled() == true);
    REQUIRE(btnRemoveParameter->isEnabled() == false);
    REQUIRE(params.getIds().size() == 0);
    REQUIRE(params.getNames().size() == 0);
    // add parameter
    mwt.addUserAction({"p", " ", "!"});
    mwt.start();
    sendMouseClick(btnAddParameter);
    REQUIRE(listParameters->count() == 1);
    REQUIRE(listParameters->currentItem()->text() == "p !");
    REQUIRE(txtParameterName->text() == "p !");
    REQUIRE(btnAddParameter->isEnabled() == true);
    REQUIRE(btnRemoveParameter->isEnabled() == true);
    REQUIRE(params.getIds().size() == 1);
    REQUIRE(params.getIds()[0] == "p_");
    REQUIRE(params.getNames()[0] == "p !");
    // edit name
    txtParameterName->setFocus();
    sendKeyEvents(txtParameterName, {"z", "Enter"});
    REQUIRE(txtParameterName->text() == "p !z");
    REQUIRE(listParameters->currentItem()->text() == "p !z");
    REQUIRE(params.getIds()[0] == "p_");
    REQUIRE(params.getNames()[0] == "p !z");
    REQUIRE(params.getName("p_") == "p !z");
    // edit expression
    txtExpression->setFocus();
    sendKeyEvents(txtExpression, {"Delete", "Backspace", "2"});
    REQUIRE(txtExpression->getMath() == "2");
    REQUIRE(params.getExpression("p_") == "2");
    // edit expression
    txtExpression->setFocus();
    sendKeyEvents(txtExpression, {"+", "c", "o", "s", "(", "1", ")"});
    REQUIRE(txtExpression->getMath() == "2 + cos(1)");
    REQUIRE(params.getExpression("p_") == "2 + cos(1)");
    // add parameter
    mwt.addUserAction({"q"});
    mwt.start();
    sendMouseClick(btnAddParameter);
    REQUIRE(listParameters->count() == 2);
    REQUIRE(params.getIds().size() == 2);
    REQUIRE(listParameters->currentItem()->text() == "q");
    // remove parameter & cancel
    sendMouseClick(btnRemoveParameter);
    sendKeyEventsToNextQDialog({"Esc"});
    REQUIRE(listParameters->count() == 2);
    REQUIRE(params.getIds().size() == 2);
    REQUIRE(listParameters->currentItem()->text() == "q");
    // remove parameter & confirm
    sendMouseClick(btnRemoveParameter);
    sendKeyEventsToNextQDialog({"Enter"});
    REQUIRE(listParameters->count() == 1);
    REQUIRE(params.getIds().size() == 1);
    REQUIRE(listParameters->currentItem()->text() == "p !z");
    // remove parameter & confirm
    sendMouseClick(btnRemoveParameter);
    sendKeyEventsToNextQDialog({"Enter"});
    REQUIRE(listParameters->count() == 0);
    REQUIRE(params.getIds().size() == 0);
  }
}

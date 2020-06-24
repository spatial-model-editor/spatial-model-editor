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
#include "tabfunctions.hpp"

SCENARIO("Functions Tab", "[gui/tabs/functions][gui/tabs][gui][functions]") {
  model::Model model;
  auto tab = TabFunctions(model);
  ModalWidgetTimer mwt;
  // get pointers to widgets within tab
  auto *listFunctions = tab.findChild<QListWidget *>("listFunctions");
  auto *btnAddFunction = tab.findChild<QPushButton *>("btnAddFunction");
  auto *btnRemoveFunction = tab.findChild<QPushButton *>("btnRemoveFunction");
  auto *txtFunctionName = tab.findChild<QLineEdit *>("txtFunctionName");
  auto *listFunctionParams = tab.findChild<QListWidget *>("listFunctionParams");
  auto *btnAddFunctionParam =
      tab.findChild<QPushButton *>("btnAddFunctionParam");
  auto *btnRemoveFunctionParam =
      tab.findChild<QPushButton *>("btnRemoveFunctionParam");
  auto *txtFunctionDef = tab.findChild<QPlainTextMathEdit *>("txtFunctionDef");
  REQUIRE(txtFunctionDef != nullptr);
  GIVEN("model with no functions") {
    if (QFile f(":/models/very-simple-model.xml");
        f.open(QIODevice::ReadOnly)) {
      model.importSBMLString(f.readAll().toStdString());
    }
    tab.loadModelData();
    REQUIRE(listFunctions->count() == 0);
    REQUIRE(btnAddFunction->isEnabled() == true);
    REQUIRE(btnRemoveFunction->isEnabled() == false);
    REQUIRE(txtFunctionName->text() == "");
    REQUIRE(listFunctionParams->count() == 0);
    REQUIRE(btnAddFunctionParam->isEnabled() == false);
    REQUIRE(btnRemoveFunctionParam->isEnabled() == false);
    REQUIRE(txtFunctionDef->getMath() == "");
    REQUIRE(model.getFunctions().getIds().isEmpty());

    // add a function
    mwt.addUserAction({"f", "u", "n", "c", " ", "1"});
    mwt.start();
    sendMouseClick(btnAddFunction);
    REQUIRE(listFunctions->count() == 1);
    REQUIRE(listFunctions->item(0)->text().toStdString() == "func 1");
    REQUIRE(btnRemoveFunction->isEnabled() == true);
    REQUIRE(model.getFunctions().getIds().size() == 1);
    REQUIRE(model.getFunctions().getIds()[0] == "func_1");
    REQUIRE(model.getFunctions().getArguments("func_1").isEmpty());
    REQUIRE(model.getFunctions().getExpression("func_1") == "0");

    // add two function params
    mwt.addUserAction({"y"});
    mwt.start();
    sendMouseClick(btnAddFunctionParam);
    REQUIRE(listFunctionParams->count() == 1);
    REQUIRE(listFunctionParams->item(0)->text().toStdString() == "y");
    REQUIRE(btnRemoveFunctionParam->isEnabled() == true);
    REQUIRE(model.getFunctions().getArguments("func_1").size() == 1);
    REQUIRE(model.getFunctions().getArguments("func_1")[0] == "y");
    mwt.addUserAction({"q", "q", " ", "Q", "A"});
    mwt.start();
    sendMouseClick(btnAddFunctionParam);
    REQUIRE(listFunctionParams->count() == 2);
    REQUIRE(listFunctionParams->item(0)->text().toStdString() == "y");
    REQUIRE(listFunctionParams->item(1)->text().toStdString() == "qq QA");
    REQUIRE(btnRemoveFunctionParam->isEnabled() == true);
    REQUIRE(model.getFunctions().getArguments("func_1").size() == 2);
    REQUIRE(model.getFunctions().getArguments("func_1")[0] == "y");
    REQUIRE(model.getFunctions().getArguments("func_1")[1] == "qq QA");

    // click remove param, then press escape at confirmation dialog to cancel
    sendMouseClick(btnRemoveFunctionParam);
    sendKeyEventsToNextQDialog({"Esc"});
    REQUIRE(listFunctionParams->count() == 2);
    REQUIRE(model.getFunctions().getArguments("func_1").size() == 2);
    REQUIRE(model.getFunctions().getArguments("func_1")[0] == "y");
    REQUIRE(model.getFunctions().getArguments("func_1")[1] == "qq QA");

    // this time press spacebar to press default 'yes' button to confirm
    sendMouseClick(btnRemoveFunctionParam);
    sendKeyEventsToNextQDialog({"Enter"});
    REQUIRE(listFunctionParams->count() == 1);
    REQUIRE(listFunctionParams->item(0)->text().toStdString() == "y");
    REQUIRE(model.getFunctions().getArguments("func_1").size() == 1);
    REQUIRE(model.getFunctions().getArguments("func_1")[0] == "y");

    // delete existing "0" to have empty expression
    // expression invalid so model not changed:
    REQUIRE(model.getFunctions().getExpression("func_1") == "0");
    // edit expression: x is not a parameter, model still not changed
    sendKeyEvents(txtFunctionDef, {"x"});
    REQUIRE(model.getFunctions().getExpression("func_1") == "0");
    // edit expression: but y is, so model updated
    sendKeyEvents(txtFunctionDef, {"Delete", "Backspace", "y"});
    REQUIRE(model.getFunctions().getExpression("func_1") == "y");

    // click remove function, then cancel
    sendMouseClick(btnRemoveFunction);
    sendKeyEventsToNextQDialog({"Esc"});
    REQUIRE(listFunctions->count() == 1);
    REQUIRE(model.getFunctions().getIds().size() == 1);
    REQUIRE(model.getFunctions().getIds()[0] == "func_1");

    // click remove function, and confirm
    sendMouseClick(btnRemoveFunction);
    sendKeyEventsToNextQDialog({"Enter"});
    REQUIRE(listFunctions->count() == 0);
    REQUIRE(listFunctionParams->count() == 0);
    REQUIRE(model.getFunctions().getIds().isEmpty());
  }
  GIVEN("circadian clock model") {
    if (QFile f(":/models/circadian-clock.xml"); f.open(QIODevice::ReadOnly)) {
      model.importSBMLString(f.readAll().toStdString());
    }
    tab.loadModelData();
    REQUIRE(listFunctions->count() == 3);
  }
}

#include <QDebug>
#include <QDialog>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>

#include "catch_wrapper.hpp"
#include "model.hpp"
#include "qplaintextmathedit.hpp"
#include "qt_test_utils.hpp"
#include "tabfunctions.hpp"

TEST_CASE("Functions Tab", "[gui/tabs/functions][gui/tabs][gui][functions]") {
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
  auto *lblFunctionDefStatus = tab.findChild<QLabel *>("lblFunctionDefStatus");
  REQUIRE(txtFunctionDef != nullptr);
  SECTION("model with no functions") {
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
    REQUIRE(txtFunctionDef->toPlainText() == "");
    REQUIRE(lblFunctionDefStatus->text() == "");
    REQUIRE(model.getFunctions().getIds().isEmpty());

    // click add function, then cancel: no-op
    mwt.addUserAction({"Esc"});
    mwt.start();
    sendMouseClick(btnAddFunction);
    REQUIRE(listFunctions->count() == 0);

    // click add function, then enter, i.e. provide no name: no-op
    mwt.addUserAction({"Enter"});
    mwt.start();
    sendMouseClick(btnAddFunction);
    REQUIRE(listFunctions->count() == 0);

    // click add a function, "func 1"
    mwt.addUserAction({"f", "u", "n", "c", " ", "1"});
    mwt.start();
    sendMouseClick(btnAddFunction);
    REQUIRE(listFunctions->count() == 1);
    REQUIRE(listFunctions->currentItem()->text() == "func 1");
    REQUIRE(listFunctions->item(0)->text() == "func 1");
    REQUIRE(btnAddFunction->isEnabled() == true);
    REQUIRE(btnRemoveFunction->isEnabled() == true);
    REQUIRE(txtFunctionName->text() == "func 1");
    REQUIRE(listFunctionParams->count() == 0);
    REQUIRE(btnAddFunctionParam->isEnabled() == true);
    REQUIRE(btnRemoveFunctionParam->isEnabled() == false);
    REQUIRE(txtFunctionDef->toPlainText() == "0");
    REQUIRE(lblFunctionDefStatus->text() == "");
    REQUIRE(model.getFunctions().getIds().size() == 1);
    REQUIRE(model.getFunctions().getIds()[0] == "func_1");
    REQUIRE(model.getFunctions().getArguments("func_1").isEmpty());
    REQUIRE(model.getFunctions().getExpression("func_1") == "0");

    // add another function, same name: "func 1"
    mwt.addUserAction({"f", "u", "n", "c", " ", "1"});
    mwt.start();
    sendMouseClick(btnAddFunction);
    REQUIRE(listFunctions->count() == 2);
    // function name changed to "func 1_" since "func 1" already exists
    REQUIRE(listFunctions->currentItem()->text() == "func 1_");
    REQUIRE(model.getFunctions().getIds().size() == 2);
    REQUIRE(model.getFunctions().getIds()[0] == "func_1");
    REQUIRE(model.getFunctions().getIds()[1] == "func_1_");

    // click remove function, and confirm
    sendMouseClick(btnRemoveFunction);
    sendKeyEventsToNextQDialog({"Enter"});
    REQUIRE(listFunctions->count() == 1);
    REQUIRE(listFunctionParams->count() == 0);
    REQUIRE(model.getFunctions().getIds().size() == 1);
    REQUIRE(model.getFunctions().getIds()[0] == "func_1");
    REQUIRE(model.getFunctions().getArguments("func_1").isEmpty());
    REQUIRE(model.getFunctions().getExpression("func_1") == "0");

    // add parameter "y"
    mwt.addUserAction({"y"});
    mwt.start();
    sendMouseClick(btnAddFunctionParam);
    REQUIRE(listFunctionParams->count() == 1);
    REQUIRE(listFunctionParams->currentItem()->text() == "y");
    REQUIRE(listFunctionParams->item(0)->text() == "y");
    REQUIRE(btnRemoveFunctionParam->isEnabled() == true);
    REQUIRE(model.getFunctions().getArguments("func_1").size() == 1);
    REQUIRE(model.getFunctions().getArguments("func_1")[0] == "y");

    // add parameter "qq QA!"
    mwt.addUserAction({"q", "q", " ", "Q", "A", "!"});
    mwt.start();
    sendMouseClick(btnAddFunctionParam);
    REQUIRE(listFunctionParams->count() == 2);
    // "qq QA!" was converted to "qqQA" to make it a valid argument name
    REQUIRE(listFunctionParams->currentItem()->text() == "qqQA");
    REQUIRE(listFunctionParams->item(0)->text() == "y");
    REQUIRE(listFunctionParams->item(1)->text() == "qqQA");
    REQUIRE(btnRemoveFunctionParam->isEnabled() == true);
    REQUIRE(model.getFunctions().getArguments("func_1").size() == 2);
    REQUIRE(model.getFunctions().getArguments("func_1")[0] == "y");
    REQUIRE(model.getFunctions().getArguments("func_1")[1] == "qqQA");

    // add parameter "y"
    mwt.addUserAction({"y"});
    mwt.start();
    sendMouseClick(btnAddFunctionParam);
    REQUIRE(listFunctionParams->count() == 3);
    // "y" was converted to "y_" as "y" already existed in the arguments
    REQUIRE(listFunctionParams->currentItem()->text() == "y_");
    REQUIRE(listFunctionParams->item(0)->text() == "y");
    REQUIRE(listFunctionParams->item(1)->text() == "qqQA");
    REQUIRE(listFunctionParams->item(2)->text() == "y_");
    REQUIRE(btnRemoveFunctionParam->isEnabled() == true);
    REQUIRE(model.getFunctions().getArguments("func_1").size() == 3);
    REQUIRE(model.getFunctions().getArguments("func_1")[0] == "y");
    REQUIRE(model.getFunctions().getArguments("func_1")[1] == "qqQA");
    REQUIRE(model.getFunctions().getArguments("func_1")[2] == "y_");

    // click remove param, then press escape at confirmation dialog to cancel
    sendMouseClick(btnRemoveFunctionParam);
    sendKeyEventsToNextQDialog({"Esc"});
    REQUIRE(listFunctionParams->count() == 3);
    REQUIRE(model.getFunctions().getArguments("func_1").size() == 3);
    REQUIRE(model.getFunctions().getArguments("func_1")[0] == "y");
    REQUIRE(model.getFunctions().getArguments("func_1")[1] == "qqQA");
    REQUIRE(model.getFunctions().getArguments("func_1")[2] == "y_");

    // click remove param, then press enter at confirmation dialog to confirm
    sendMouseClick(btnRemoveFunctionParam);
    sendKeyEventsToNextQDialog({"Enter"});
    REQUIRE(listFunctionParams->count() == 2);
    REQUIRE(listFunctionParams->currentItem()->text() == "qqQA");
    REQUIRE(model.getFunctions().getArguments("func_1").size() == 2);
    REQUIRE(model.getFunctions().getArguments("func_1")[0] == "y");
    REQUIRE(model.getFunctions().getArguments("func_1")[1] == "qqQA");

    // click remove param, then press enter at confirmation dialog to confirm
    sendMouseClick(btnRemoveFunctionParam);
    sendKeyEventsToNextQDialog({"Enter"});
    REQUIRE(listFunctionParams->count() == 1);
    REQUIRE(listFunctionParams->currentItem()->text() == "y");
    REQUIRE(model.getFunctions().getArguments("func_1").size() == 1);
    REQUIRE(model.getFunctions().getArguments("func_1")[0] == "y");

    REQUIRE(txtFunctionDef->toPlainText() == "0");
    // delete existing "0" to have empty expression
    sendKeyEvents(txtFunctionDef, {"Delete", "Backspace"});
    REQUIRE(txtFunctionDef->toPlainText() == "");
    REQUIRE(lblFunctionDefStatus->text() == "Empty expression");
    // expression invalid so model not changed:
    REQUIRE(model.getFunctions().getExpression("func_1") == "0");
    sendKeyEvents(txtFunctionDef, {"x"});
    REQUIRE(txtFunctionDef->toPlainText() == "x");
    REQUIRE(lblFunctionDefStatus->text() == "variable 'x' not found");
    // edit expression: x is not a parameter, model still not changed
    REQUIRE(model.getFunctions().getExpression("func_1") == "0");
    // edit expression: y is a parameter, so math is valid and model is updated
    sendKeyEvents(txtFunctionDef, {"Delete", "Backspace", "y"});
    REQUIRE(txtFunctionDef->toPlainText() == "y");
    REQUIRE(lblFunctionDefStatus->text() == "");
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
  SECTION("circadian clock model") {
    if (QFile f(":/models/circadian-clock.xml"); f.open(QIODevice::ReadOnly)) {
      model.importSBMLString(f.readAll().toStdString());
    }
    tab.loadModelData();
    REQUIRE(listFunctions->count() == 3);
  }
}

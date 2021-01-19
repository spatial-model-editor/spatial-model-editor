#include "catch_wrapper.hpp"
#include "model.hpp"
#include "qplaintextmathedit.hpp"
#include "qt_test_utils.hpp"
#include "tabparameters.hpp"
#include <QDebug>
#include <QDialog>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>

SCENARIO("Parameters Tab", "[gui/tabs/parameters][gui/tabs][gui][parameters]") {
  sme::model::Model model;
  auto tab = TabParameters(model);
  // get pointers to widgets within tab
  auto *listParameters = tab.findChild<QListWidget *>("listParameters");
  auto *btnAddParameter = tab.findChild<QPushButton *>("btnAddParameter");
  auto *btnRemoveParameter = tab.findChild<QPushButton *>("btnRemoveParameter");
  auto *txtParameterName = tab.findChild<QLineEdit *>("txtParameterName");
  auto *txtExpression = tab.findChild<QPlainTextMathEdit *>("txtExpression");
  auto *lblExpressionStatus = tab.findChild<QLabel *>("lblExpressionStatus");
  const auto &params = model.getParameters();
  REQUIRE(txtExpression != nullptr);
  ModalWidgetTimer mwt;
  GIVEN("very simple model") {
    if (QFile f(":/models/very-simple-model.xml");
        f.open(QIODevice::ReadOnly)) {
      model.importSBMLString(f.readAll().toStdString());
    }
    tab.loadModelData();
    REQUIRE(params.getIds().size() == 1);
    REQUIRE(params.getIds()[0] == "param");
    REQUIRE(listParameters->count() == 1);
    REQUIRE(btnAddParameter->isEnabled() == true);
    REQUIRE(btnRemoveParameter->isEnabled() == true);
    REQUIRE(txtParameterName->isEnabled() == true);
    REQUIRE(txtParameterName->text() == params.getName("param"));
    REQUIRE(txtExpression->isEnabled() == true);
    REQUIRE(txtExpression->toPlainText() == params.getExpression("param"));
    REQUIRE(lblExpressionStatus->text() == "");
    // remove parameter & confirm
    sendMouseClick(btnRemoveParameter);
    sendKeyEventsToNextQDialog({"Enter"});
    REQUIRE(listParameters->count() == 0);
    REQUIRE(btnAddParameter->isEnabled() == true);
    REQUIRE(btnRemoveParameter->isEnabled() == false);
    REQUIRE(txtParameterName->isEnabled() == false);
    REQUIRE(txtParameterName->text() == "");
    REQUIRE(txtExpression->isEnabled() == false);
    REQUIRE(txtExpression->toPlainText() == "");
    REQUIRE(lblExpressionStatus->text() == "");
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
    REQUIRE(txtExpression->toPlainText() == "0");
    // expression -> ""
    txtExpression->setFocus();
    sendKeyEvents(txtExpression, {"Delete", "Backspace"});
    REQUIRE(txtExpression->toPlainText() == "");
    REQUIRE(lblExpressionStatus->text() == "Empty expression");
    // invalid expression so model expression unchanged
    REQUIRE(params.getExpression("p_") == "0");
    // expression -> "2"
    sendKeyEvents(txtExpression, {"2"});
    REQUIRE(txtExpression->toPlainText() == "2");
    REQUIRE(lblExpressionStatus->text() == "");
    REQUIRE(params.getExpression("p_") == "2");
    // expression -> "2+cos(1)"
    sendKeyEvents(txtExpression, {"+", "c", "o", "s", "(", "1", ")"});
    REQUIRE(txtExpression->toPlainText() == "2+cos(1)");
    REQUIRE(lblExpressionStatus->text() == "");
    REQUIRE(params.getExpression("p_") == "2 + cos(1)");
    // add parameter with same name
    mwt.addUserAction({"p", " ", "!", "z"});
    mwt.start();
    sendMouseClick(btnAddParameter);
    REQUIRE(listParameters->count() == 2);
    REQUIRE(params.getIds().size() == 2);
    // name changed to make it unique
    REQUIRE(listParameters->currentItem()->text() == "p !z_");
    // remove parameter & cancel
    sendMouseClick(btnRemoveParameter);
    sendKeyEventsToNextQDialog({"Esc"});
    REQUIRE(listParameters->count() == 2);
    REQUIRE(params.getIds().size() == 2);
    REQUIRE(listParameters->currentItem()->text() == "p !z_");
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

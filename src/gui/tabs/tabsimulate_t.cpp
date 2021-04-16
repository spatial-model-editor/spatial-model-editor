#include "catch_wrapper.hpp"
#include "model.hpp"
#include "qlabelmousetracker.hpp"
#include "qt_test_utils.hpp"
#include "tabsimulate.hpp"
#include <QFile>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>
#include <qcustomplot.h>

SCENARIO("Simulate Tab", "[gui/tabs/simulate][gui/tabs][gui][simulate]") {
  sme::model::Model sbmlDoc;
  QLabelMouseTracker mouseTracker;
  auto tab = TabSimulate(sbmlDoc, &mouseTracker);
  tab.show();
  waitFor(&tab);
  ModalWidgetTimer mwt;
  // get pointers to widgets within tab
  auto *txtSimLength{tab.findChild<QLineEdit *>("txtSimLength")};
  REQUIRE(txtSimLength != nullptr);
  auto *txtSimInterval{tab.findChild<QLineEdit *>("txtSimInterval")};
  REQUIRE(txtSimInterval != nullptr);
  auto *btnSimulate{tab.findChild<QPushButton *>("btnSimulate")};
  REQUIRE(btnSimulate != nullptr);
  auto *btnResetSimulation{tab.findChild<QPushButton *>("btnResetSimulation")};
  REQUIRE(btnResetSimulation != nullptr);
  auto *hslideTime{tab.findChild<QSlider *>("hslideTime")};
  REQUIRE(hslideTime != nullptr);
  auto *btnExport{tab.findChild<QPushButton *>("btnExport")};
  REQUIRE(btnExport != nullptr);
  auto *btnDisplayOptions{tab.findChild<QPushButton *>("btnDisplayOptions")};
  REQUIRE(btnDisplayOptions != nullptr);

  if (QFile f(":/models/ABtoC.xml"); f.open(QIODevice::ReadOnly)) {
    sbmlDoc.importSBMLString(f.readAll().toStdString());
  }
  tab.loadModelData();
  txtSimLength->setFocus();

  // do DUNE simulation with two timesteps of 0.1
  REQUIRE(hslideTime->isEnabled() == true);
  sendKeyEvents(txtSimLength,
                {"End", "Backspace", "Backspace", "Backspace", "0", ".", "2"});
  sendKeyEvents(txtSimInterval,
                {"End", "Backspace", "Backspace", "Backspace", "0", ".", "1"});
  sendMouseClick(btnSimulate);
  REQUIRE(btnSimulate->isEnabled() == false);
  // simulation happens asynchronously - wait until finished
  while (!btnSimulate->isEnabled()) {
    wait(100);
  }
  REQUIRE(btnSimulate->isEnabled() == true);
  REQUIRE(hslideTime->isEnabled() == true);
  REQUIRE(hslideTime->minimum() == 0);

  // reset simulation
  sendMouseClick(btnResetSimulation);
  REQUIRE(hslideTime->isEnabled() == true);
  REQUIRE(btnSimulate->isEnabled() == true);

  // new simulation using Pixel simulator
  tab.useDune(false);
  sendMouseClick(btnSimulate);
  REQUIRE(btnSimulate->isEnabled() == false);
  while (!btnSimulate->isEnabled()) {
    wait(100);
  }
  REQUIRE(hslideTime->isEnabled() == true);
  REQUIRE(hslideTime->maximum() == 2);
  REQUIRE(hslideTime->minimum() == 0);

  sendMouseClick(btnResetSimulation);
  REQUIRE(hslideTime->isEnabled() == true);

  // start new sim but click cancel straight away
  // cancel simulation early
  mwt.addUserAction({"Escape"});
  mwt.start();
  sendMouseClick(btnSimulate);
  // wait until simulation stops
  while (!btnSimulate->isEnabled()) {
    wait(100);
  }

  // set an invalid simulation lengths / image intervals
  sendMouseClick(btnResetSimulation);
  QString invalidMessage{"Invalid simulation length or image interval"};
  // invalid double
  sendKeyEvents(txtSimLength, {";", "c"});
  mwt.start();
  sendMouseClick(btnSimulate);
  REQUIRE(mwt.getResult() == invalidMessage);

  // do valid multiple timestep simulation: 2x0.5, 2x0.25
  sendKeyEvents(txtSimLength,
                {"Backspace", "Backspace", "Backspace", "Backspace",
                 "Backspace", "Backspace", "Backspace", "Backspace",
                 "Backspace", "1", ";", "0", ".", "5"});
  sendKeyEvents(txtSimInterval,
                {"Backspace", "Backspace", "Backspace", "Backspace",
                 "Backspace", "Backspace", "Backspace", "0", ".", "4", "9", "9", "9", ";", "0",
                 ".", "2", "5", "0", "1"});
  sendMouseClick(btnSimulate);
  REQUIRE(btnSimulate->isEnabled() == false);
  while (!btnSimulate->isEnabled()) {
    wait(100);
  }
  // image intervals -> closest valid values (i.e. integer number of steps)
  REQUIRE(txtSimInterval->text() == "0.5;0.25");
  REQUIRE(hslideTime->isEnabled() == true);
  REQUIRE(hslideTime->maximum() == 4);
  REQUIRE(hslideTime->minimum() == 0);

  // hide all species
  mwt.addUserAction({"Space"});
  mwt.start();
  sendMouseClick(btnDisplayOptions);

  // click export & cancel
  mwt.addUserAction({"Esc"});
  mwt.start();
  sendMouseClick(btnExport);
  REQUIRE(mwt.getResult() == "Export simulation results");
}

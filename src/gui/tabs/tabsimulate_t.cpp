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
  auto *txtSimLength = tab.findChild<QLineEdit *>("txtSimLength");
  auto *txtSimInterval = tab.findChild<QLineEdit *>("txtSimInterval");
  auto *txtSimDt = tab.findChild<QLineEdit *>("txtSimDt");
  auto *btnSimulate = tab.findChild<QPushButton *>("btnSimulate");
  auto *btnResetSimulation = tab.findChild<QPushButton *>("btnResetSimulation");
  auto *hslideTime = tab.findChild<QSlider *>("hslideTime");
  auto *btnExport = tab.findChild<QPushButton *>("btnExport");
  auto *btnDisplayOptions = tab.findChild<QPushButton *>("btnDisplayOptions");

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
  sendKeyEvents(txtSimDt, {"End", "Backspace", "Backspace", "Backspace",
                           "Backspace", "0", ".", "1"});
  sendMouseClick(btnSimulate);
  REQUIRE(btnSimulate->isEnabled() == false);
  // simulation happens asynchronously - wait until finished
  while (!btnSimulate->isEnabled()) {
    wait(100);
  }
  REQUIRE(btnSimulate->isEnabled() == true);
  REQUIRE(hslideTime->isEnabled() == true);
  REQUIRE(hslideTime->minimum() == 0);

  sendMouseClick(btnResetSimulation);
  REQUIRE(hslideTime->isEnabled() == true);
  REQUIRE(btnSimulate->isEnabled() == true);

  // repeat simulation using Pixel simulator
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

  // repeat sim but click cancel straight away
  // cancel simulation early
  mwt.addUserAction({"Escape"});
  mwt.start();
  sendMouseClick(btnSimulate);
  // wait until simulation stops
  while (!btnSimulate->isEnabled()) {
    wait(100);
  }

  // hide all species
  mwt.addUserAction({"Space"});
  mwt.start();
  sendMouseClick(btnDisplayOptions);

  // click save image & cancel
  mwt.addUserAction({"Esc"});
  mwt.start();
  sendMouseClick(btnExport);
  REQUIRE(mwt.getResult() == "Export simulation results");
}

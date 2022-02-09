#include "catch_wrapper.hpp"
#include "model_test_utils.hpp"
#include "qlabelmousetracker.hpp"
#include "qt_test_utils.hpp"
#include "sme/model.hpp"
#include "tabsimulate.hpp"
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>

using namespace sme::test;

TEST_CASE("TabSimulate", "[gui/tabs/simulate][gui/tabs][gui][simulate]") {
  // load model & do initial simulation
  auto model{getExampleModel(Mod::ABtoC)};
  model.getSimulationSettings().times.clear();
  sme::simulate::Simulation sim(model);
  sim.doMultipleTimesteps({{2, 0.01}, {1, 0.02}});

  QLabelMouseTracker mouseTracker;
  TabSimulate tab(model, &mouseTracker);
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

  // load existing simulation data
  tab.importTimesAndIntervalsOnNextLoad();
  tab.loadModelData();
  REQUIRE(txtSimLength->text() == "0.02;0.02");
  REQUIRE(txtSimInterval->text() == "0.01;0.02");
  REQUIRE(hslideTime->minimum() == 0);
  REQUIRE(hslideTime->maximum() == 3);

  // change timesteps
  txtSimLength->setFocus();
  sendKeyEvents(txtSimLength, {"End", "Backspace", "Backspace", "Backspace",
                               "Backspace", "Backspace", "9"});
  sendKeyEvents(txtSimInterval, {"End", "Backspace", "Backspace", "Backspace",
                                 "Backspace", "Backspace", "1"});
  REQUIRE(txtSimLength->text() == "0.029");
  REQUIRE(txtSimInterval->text() == "0.011");

  // reset simulation: resets times to original ones
  sendMouseClick(btnResetSimulation);
  REQUIRE(txtSimLength->text() == "0.02;0.02");
  REQUIRE(txtSimInterval->text() == "0.01;0.02");
  REQUIRE(hslideTime->isEnabled() == true);
  REQUIRE(btnSimulate->isEnabled() == true);

  // do DUNE simulation with two timesteps of 0.1
  REQUIRE(hslideTime->isEnabled() == true);
  txtSimLength->setFocus();
  sendKeyEvents(txtSimLength,
                {"End", "Backspace", "Backspace", "Backspace", "Backspace",
                 "Backspace", "Backspace", "Backspace", "Backspace",
                 "Backspace", "Backspace", "Backspace", "Backspace", "0", ".",
                 "2"});
  sendKeyEvents(txtSimInterval,
                {"End", "Backspace", "Backspace", "Backspace", "Backspace",
                 "Backspace", "Backspace", "Backspace", "Backspace",
                 "Backspace", "Backspace", "Backspace", "Backspace", "0", ".",
                 "1"});
  REQUIRE(txtSimLength->text() == "0.2");
  REQUIRE(txtSimInterval->text() == "0.1");
  sendMouseClick(btnSimulate);
  REQUIRE(btnSimulate->isEnabled() == false);
  // simulation happens asynchronously - wait until finished
  while (!btnSimulate->isEnabled()) {
    wait(100);
  }
  REQUIRE(btnSimulate->isEnabled() == true);
  REQUIRE(hslideTime->isEnabled() == true);
  REQUIRE(hslideTime->minimum() == 0);
  REQUIRE(hslideTime->maximum() == 2);

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
                 "Backspace", "Backspace", "Backspace", "0",
                 ".",         "4",         "9",         "9",
                 "9",         ";",         "0",         ".",
                 "2",         "5",         "0",         "1"});
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

  // make a species non-spatial & try to use dune
  model.getSpecies().setIsSpatial("A", false);
  REQUIRE(model.getSpecies().getIsSpatial("A") == false);
  mwt.addUserAction({"Esc"});
  mwt.start();
  tab.useDune(true);
  QString invalidDune{"The model contains non-spatial species, which are not "
                      "currently supported by DuneCopasi. Would you like to "
                      "use the Pixel simulator instead?"};
  REQUIRE(mwt.getResult() == invalidDune);
}

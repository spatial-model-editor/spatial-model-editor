#include <qcustomplot.h>

#include <QFile>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>

#include "catch_wrapper.hpp"
#include "qlabelmousetracker.hpp"
#include "qt_test_utils.hpp"
#include "sbml.hpp"
#include "tabsimulate.hpp"

SCENARIO("Simulate Tab", "[gui][tabs][simulate]") {
  sbml::SbmlDocWrapper sbmlDoc;
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
  auto *pltPlot = tab.findChild<QCustomPlot *>("pltPlot");
  auto *btnDisplayOptions = tab.findChild<QPushButton *>("btnDisplayOptions");
  REQUIRE(pltPlot != nullptr);

  if (QFile f(":/models/ABtoC.xml"); f.open(QIODevice::ReadOnly)) {
    sbmlDoc.importSBMLString(f.readAll().toStdString());
  }
  tab.loadModelData();
  txtSimLength->setFocus();

  // do DUNE simulation with two timesteps of 0.1
  REQUIRE(hslideTime->isEnabled() == true);
  REQUIRE(pltPlot->graphCount() == 9);
  sendKeyEvents(txtSimLength,
                {"End", "Backspace", "Backspace", "Backspace", "0", ".", "2"});
  sendKeyEvents(txtSimInterval,
                {"End", "Backspace", "Backspace", "Backspace", "0", ".", "1"});
  sendKeyEvents(txtSimDt, {"End", "Backspace", "Backspace", "Backspace",
                           "Backspace", "0", ".", "1"});
  sendMouseClick(btnSimulate);
  REQUIRE(hslideTime->isEnabled() == true);
  REQUIRE(hslideTime->maximum() == 2);
  REQUIRE(hslideTime->minimum() == 0);
  REQUIRE(pltPlot->graphCount() == 9);

  sendMouseClick(btnResetSimulation);
  REQUIRE(hslideTime->isEnabled() == true);
  REQUIRE(pltPlot->graphCount() == 9);

  // hide all species
  mwt.addUserAction({"Space"});
  mwt.start();
  sendMouseClick(btnDisplayOptions);
  REQUIRE(pltPlot->graph(0)->visible() == false);
  REQUIRE(pltPlot->graph(3)->visible() == false);
  REQUIRE(pltPlot->graph(6)->visible() == false);

  // only show species B
  mwt.addUserAction({"Down", "Down", "Space"});
  mwt.start();
  sendMouseClick(btnDisplayOptions);
  REQUIRE(pltPlot->graph(0)->visible() == false);
  REQUIRE(pltPlot->graph(3)->visible() == true);
  REQUIRE(pltPlot->graph(6)->visible() == false);

  // repeat simulation using Pixel simulator
  tab.useDune(false);
  sendMouseClick(btnSimulate);
  REQUIRE(hslideTime->isEnabled() == true);
  REQUIRE(hslideTime->maximum() == 2);
  REQUIRE(hslideTime->minimum() == 0);
  REQUIRE(pltPlot->graphCount() == 9);

  sendMouseClick(btnResetSimulation);
  REQUIRE(hslideTime->isEnabled() == true);
  REQUIRE(pltPlot->graphCount() == 9);
}

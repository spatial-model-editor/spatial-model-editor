#include "catch_wrapper.hpp"
#include "plotwrapper.hpp"
#include "utils.hpp"

SCENARIO("PlotWrapper",
         "[gui/widgets/plotwrapper][gui/widgets][gui][plotwrapper]") {
  QWidget parent;
  PlotWrapper p("title", &parent);
  REQUIRE(p.plot->graphCount() == 0);
  // add two species
  p.addAvMinMaxLine("s1", sme::utils::indexedColours()[0]);
  REQUIRE(p.plot->graph(0)->name() == "s1 (average)");
  REQUIRE(p.plot->graph(1)->name() == "s1 (min/max range)");
  p.addAvMinMaxLine("s2", sme::utils::indexedColours()[1]);
  REQUIRE(p.plot->graph(3)->name() == "s2 (average)");
  REQUIRE(p.plot->graph(4)->name() == "s2 (min/max range)");
  REQUIRE(p.plot->graphCount() == 6);
  REQUIRE(p.plot->legend->itemCount() == 4);
  for (int i = 0; i < 6; ++i) {
    REQUIRE(p.plot->graph(i)->dataCount() == 0);
  }

  // add two data points
  p.addAvMinMaxPoint(0, 0.0, {1.0, 0.95, 1.05});
  p.addAvMinMaxPoint(1, 0.0, {2.0, 1.95, 2.05});
  for (int i = 0; i < 6; ++i) {
    REQUIRE(p.plot->graph(i)->dataCount() == 1);
  }
  p.addAvMinMaxPoint(0, 1.0, {1.1, 0.99, 1.15});
  p.addAvMinMaxPoint(1, 1.0, {0.9, 0.85, 1.01});
  for (int i = 0; i < 6; ++i) {
    REQUIRE(p.plot->graph(i)->dataCount() == 2);
  }
  for (int i = 0; i < 6; ++i) {
    REQUIRE(p.plot->graph(i)->visible() == true);
  }

  // hide all species
  p.update({false, false}, false);
  for (int i = 0; i < 6; ++i) {
    REQUIRE(p.plot->graph(i)->visible() == false);
  }
  REQUIRE(p.plot->legend->itemCount() == 0);

  // only show avg for all species
  p.update({true, true}, false);
  for (int i = 0; i < 6; ++i) {
    REQUIRE(p.plot->graph(i)->visible() == (i % 3 == 0));
  }
  REQUIRE(p.plot->legend->itemCount() == 2);

  // show all for 1st spec, hide 2nd spec
  p.update({true, false}, true);
  for (int i = 0; i < 6; ++i) {
    REQUIRE(p.plot->graph(i)->visible() == (i < 3));
  }
  REQUIRE(p.plot->legend->itemCount() == 2);

  // add a custom observable
  p.addObservableLine({"1", "1", true}, sme::utils::indexedColours()[2]);
  REQUIRE(p.plot->graphCount() == 7);
  REQUIRE(p.plot->graph(6)->name() == "1");
  for (int i = 0; i < 6; ++i) {
    REQUIRE(p.plot->graph(i)->visible() == (i < 3));
  }
  REQUIRE(p.plot->graph(6)->visible() == true);
  REQUIRE(p.plot->legend->itemCount() == 3);

  // add a custom observable
  p.addObservableLine({"s1 + s2", "s1 + s2", true}, sme::utils::indexedColours()[3]);
  REQUIRE(p.plot->graphCount() == 8);

  // make all species visible
  p.update({true, true}, true);
  for (int i = 0; i < 8; ++i) {
    REQUIRE(p.plot->graph(i)->visible() == true);
  }

  // get timepoints
  auto times = p.getTimepoints();
  REQUIRE(times.size() == 2);

  // export CSV
  auto csv = p.getDataAsCSV().split("\n");
  REQUIRE(csv.size() == 4);
  REQUIRE(csv[0] ==
          "time, s1 (avg), s1 (min), s1 (max), s2 (avg), s2 (min), s2 (max)");
  REQUIRE(csv[1] ==
          "0.00000000000000e+00, 1.00000000000000e+00, 9.50000000000000e-01, "
          "1.05000000000000e+00, 2.00000000000000e+00, 1.95000000000000e+00, "
          "2.05000000000000e+00");
  REQUIRE(csv[2] ==
          "1.00000000000000e+00, 1.10000000000000e+00, 9.90000000000000e-01, "
          "1.15000000000000e+00, 9.00000000000000e-01, 8.50000000000000e-01, "
          "1.01000000000000e+00");
  REQUIRE(csv[3] == "");

  // reset custom observables
  p.clearObservableLines();
  REQUIRE(p.plot->graphCount() == 6);

  // reset everything
  p.clear();
  REQUIRE(p.plot->graphCount() == 0);
  REQUIRE(p.getTimepoints().size() == 0);
}

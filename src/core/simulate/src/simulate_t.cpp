#include "catch_wrapper.hpp"
#include "mesh.hpp"
#include "model.hpp"
#include "qt_test_utils.hpp"
#include "sbml_test_data/very_simple_model.hpp"
#include "simulate.hpp"
#include "utils.hpp"
#include <QFile>
#include <algorithm>
#include <cmath>
#include <future>
#include <sbml/SBMLDocument.h>
#include <sbml/SBMLReader.h>
#include <sbml/SBMLWriter.h>

SCENARIO("Simulate: very_simple_model, single pixel geometry",
         "[core/simulate/simulate][core/simulate][core][simulate][pixel]") {
  // import model
  std::unique_ptr<libsbml::SBMLDocument> doc(
      libsbml::readSBMLFromString(sbml_test_data::very_simple_model().xml));
  libsbml::SBMLWriter().writeSBML(doc.get(), "tmp.xml");
  model::Model s;
  s.importSBMLFile("tmp.xml");

  // import geometry & assign compartments
  QImage img(1, 3, QImage::Format_RGB32);
  QRgb col1 = QColor(12, 243, 154).rgba();
  QRgb col2 = QColor(112, 243, 154).rgba();
  QRgb col3 = QColor(212, 243, 154).rgba();
  img.setPixel(0, 0, col1);
  img.setPixel(0, 1, col2);
  img.setPixel(0, 2, col3);
  img.save("tmp.bmp");
  s.getGeometry().importGeometryFromImage(QImage("tmp.bmp"));
  s.getGeometry().setPixelWidth(1.0);
  s.getCompartments().setColour("c1", col1);
  s.getCompartments().setColour("c2", col2);
  s.getCompartments().setColour("c3", col3);
  // check we have identified the compartments and membranes
  REQUIRE(s.getCompartments().getIds() == QStringList{"c1", "c2", "c3"});
  REQUIRE(s.getMembranes().getIds() ==
          QStringList{"c1_c2_membrane", "c2_c3_membrane"});

  // check fields have correct compartments & sizes
  geometry::Field *fa1 = s.getSpecies().getField("A_c1");
  REQUIRE(fa1->getCompartment()->getId() == "c1");
  REQUIRE(fa1->getId() == "A_c1");
  geometry::Field *fb1 = s.getSpecies().getField("B_c1");
  REQUIRE(fb1->getCompartment()->getId() == "c1");
  REQUIRE(fb1->getId() == "B_c1");
  geometry::Field *fa2 = s.getSpecies().getField("A_c2");
  REQUIRE(fa2->getCompartment()->getId() == "c2");
  REQUIRE(fa2->getId() == "A_c2");
  geometry::Field *fb2 = s.getSpecies().getField("B_c2");
  REQUIRE(fb2->getCompartment()->getId() == "c2");
  REQUIRE(fb2->getId() == "B_c2");
  geometry::Field *fa3 = s.getSpecies().getField("A_c3");
  REQUIRE(fa3->getCompartment()->getId() == "c3");
  REQUIRE(fa3->getId() == "A_c3");
  geometry::Field *fb3 = s.getSpecies().getField("B_c3");
  REQUIRE(fb3->getCompartment()->getId() == "c3");
  REQUIRE(fb3->getId() == "B_c3");

  // check membranes have correct compartment pairs & sizes
  const auto &m0 = s.getMembranes().getMembranes()[0];
  REQUIRE(m0.getId() == "c1_c2_membrane");
  REQUIRE(m0.getCompartmentA()->getId() == "c1");
  REQUIRE(m0.getCompartmentB()->getId() == "c2");
  REQUIRE(m0.getIndexPairs().size() == 1);
  REQUIRE(m0.getIndexPairs()[0] == std::pair<std::size_t, std::size_t>{0, 0});

  const auto &m1 = s.getMembranes().getMembranes()[1];
  REQUIRE(m1.getId() == "c2_c3_membrane");
  REQUIRE(m1.getCompartmentA()->getId() == "c2");
  REQUIRE(m1.getCompartmentB()->getId() == "c3");
  REQUIRE(m1.getIndexPairs().size() == 1);
  REQUIRE(m1.getIndexPairs()[0] == std::pair<std::size_t, std::size_t>{0, 0});

  // move membrane reactions from compartment to membrane
  s.getReactions().setLocation("A_uptake", "c1_c2_membrane");
  s.getReactions().setLocation("A_transport", "c2_c3_membrane");
  s.getReactions().setLocation("B_transport", "c2_c3_membrane");
  s.getReactions().setLocation("B_excretion", "c1_c2_membrane");

  double dt = 0.134521234;
  // 1st order RK, multi-threaded fixed timestep simulation
  simulate::Options options;
  options.pixel.integrator = simulate::PixelIntegratorType::RK101;
  options.pixel.maxErr = {std::numeric_limits<double>::max(),
                          std::numeric_limits<double>::max()};
  options.pixel.maxTimestep = dt;
  options.pixel.enableMultiThreading = true;
  options.pixel.maxThreads = 2;
  simulate::Simulation sim(s, simulate::SimulatorType::Pixel, options);

  // check initial concentrations:
  // note: A_c1 is constant, so not part of simulation
  REQUIRE(sim.getAvgMinMax(0, 0, 0).avg == dbl_approx(0.0));
  REQUIRE(sim.getAvgMinMax(0, 1, 0).avg == dbl_approx(0.0));
  REQUIRE(sim.getAvgMinMax(0, 1, 1).avg == dbl_approx(0.0));
  REQUIRE(sim.getAvgMinMax(0, 2, 0).avg == dbl_approx(0.0));
  REQUIRE(sim.getAvgMinMax(0, 2, 1).avg == dbl_approx(0.0));

  // check initial concentration image
  img = sim.getConcImage(0);
  REQUIRE(img.size() == QSize(1, 3));
  REQUIRE(img.pixel(0, 0) == QColor(0, 0, 0).rgba());
  REQUIRE(img.pixel(0, 1) == QColor(0, 0, 0).rgba());
  REQUIRE(img.pixel(0, 2) == QColor(0, 0, 0).rgba());

  double volC1 = 10.0;
  WHEN("single Euler step") {
    auto steps = sim.doTimesteps(dt);
    REQUIRE(steps == 1);
    std::size_t it = 1;
    // B_c1 = 0
    REQUIRE(sim.getAvgMinMax(it, 0, 0).avg == dbl_approx(0.0));
    // A_c2 += k_1 A_c1 dt
    REQUIRE(sim.getAvgMinMax(it, 1, 0).avg == dbl_approx(0.1 * 1.0 * dt));
    // B_c2 = 0
    REQUIRE(sim.getAvgMinMax(it, 1, 1).avg == dbl_approx(0.0));
    // A_c3 = 0
    REQUIRE(sim.getAvgMinMax(it, 2, 0).avg == dbl_approx(0.0));
    // B_c3 = 0
    REQUIRE(sim.getAvgMinMax(it, 2, 1).avg == dbl_approx(0.0));
  }

  WHEN("two Euler steps") {
    auto steps = sim.doTimesteps(dt);
    REQUIRE(steps == 1);
    double A_c2 = sim.getAvgMinMax(1, 1, 0).avg;
    steps = sim.doTimesteps(dt);
    REQUIRE(steps == 1);
    std::size_t it = 2;
    // B_c1 = 0
    REQUIRE(sim.getAvgMinMax(it, 0, 0).avg == dbl_approx(0.0));
    // A_c2 += k_1 A_c1 dt - k1 * A_c2 * dt
    REQUIRE(sim.getAvgMinMax(it, 1, 0).avg ==
            dbl_approx(A_c2 + 0.1 * dt - A_c2 * 0.1 * dt));
    // B_c2 = 0
    REQUIRE(sim.getAvgMinMax(it, 1, 1).avg == dbl_approx(0.0));
    // A_c3 += k_1 A_c2 dt / c3
    REQUIRE(sim.getAvgMinMax(it, 2, 0).avg == dbl_approx(0.1 * A_c2 * dt));
    // B_c3 = 0
    REQUIRE(sim.getAvgMinMax(it, 2, 1).avg == dbl_approx(0.0));
  }

  WHEN("three Euler steps") {
    sim.doTimesteps(dt);
    sim.doTimesteps(dt);
    double A_c2 = sim.getAvgMinMax(2, 1, 0).avg;
    double A_c3 = sim.getAvgMinMax(2, 2, 0).avg;
    sim.doTimesteps(dt);
    std::size_t it = 3;
    // B_c1 = 0
    REQUIRE(sim.getAvgMinMax(it, 0, 0).avg == dbl_approx(0.0));
    // A_c2 += k_1 (A_c1 - A_c2) dt + k2 * A_c3 * dt
    REQUIRE(sim.getAvgMinMax(it, 1, 0).avg ==
            dbl_approx(A_c2 + 0.1 * dt - A_c2 * 0.1 * dt + A_c3 * 0.1 * dt));
    // B_c2 = 0
    REQUIRE(sim.getAvgMinMax(it, 1, 1).avg == dbl_approx(0.0));
    // A_c3 += k_1 A_c2 dt - k_1 A_c3 dt -
    REQUIRE(sim.getAvgMinMax(it, 2, 0).avg ==
            dbl_approx(A_c3 + (0.1 * (A_c2 - A_c3) * dt - 0.3 * A_c3 * dt)));
    // B_c3 = 0.2 * 0.3 * A_c3 * dt
    REQUIRE(sim.getAvgMinMax(it, 2, 1).avg == dbl_approx(0.3 * A_c3 * dt));
  }

  WHEN("many Euler steps -> steady state solution") {
    // when A & B saturate in all compartments, we reach a steady state
    // by conservation: flux of B of into c1 = flux of A from c1 = 0.1
    // all other net fluxes are zero
    double acceptable_error = 1.e-8;
    options.pixel.maxTimestep = 0.20138571;

    simulate::Simulation sim2(s, simulate::SimulatorType::Pixel, options);
    sim2.doTimesteps(1000);
    std::size_t it = sim2.getTimePoints().size() - 1;
    double A_c1 = 1.0;
    double A_c2 = sim2.getAvgMinMax(it, 1, 0).avg;
    double A_c3 = sim2.getAvgMinMax(it, 2, 0).avg;
    double B_c1 = sim2.getAvgMinMax(it, 0, 0).avg;
    double B_c2 = sim2.getAvgMinMax(it, 1, 1).avg;
    double B_c3 = sim2.getAvgMinMax(it, 2, 1).avg;

    // check concentration values
    REQUIRE(A_c1 == Catch::Approx(1.0).epsilon(acceptable_error));
    REQUIRE(A_c2 == Catch::Approx(0.5 * A_c1 * (0.06 + 0.10) / 0.06)
                        .epsilon(acceptable_error));
    REQUIRE(A_c3 == Catch::Approx(A_c2 - A_c1).epsilon(acceptable_error));
    // B_c1 "steady state" solution is linear growth
    REQUIRE(
        B_c3 ==
        Catch::Approx((0.06 / 0.10) * A_c3 / 0.2).epsilon(acceptable_error));
    REQUIRE(B_c2 == Catch::Approx(B_c3 / 2.0).epsilon(acceptable_error));

    // check concentration derivatives
    double eps = 1.e-5;
    sim2.doTimesteps(eps);
    ++it;
    double dA2 = (sim2.getAvgMinMax(it, 1, 0).avg - A_c2) / eps;
    REQUIRE(dA2 == Catch::Approx(0).epsilon(acceptable_error));
    double dA3 = (sim2.getAvgMinMax(it, 2, 0).avg - A_c3) / eps;
    REQUIRE(dA3 == Catch::Approx(0).epsilon(acceptable_error));
    double dB1 = volC1 * (sim2.getAvgMinMax(it, 0, 0).avg - B_c1) / eps;
    REQUIRE(dB1 == Catch::Approx(1).epsilon(acceptable_error));
    double dB2 = (sim2.getAvgMinMax(it, 1, 1).avg - B_c2) / eps;
    REQUIRE(dB2 == Catch::Approx(0).epsilon(acceptable_error));
    double dB3 = (sim2.getAvgMinMax(it, 2, 1).avg - B_c3) / eps;
    REQUIRE(dB3 == Catch::Approx(0).epsilon(acceptable_error));
  }
}

SCENARIO("Simulate: very_simple_model, 2d geometry",
         "[core/simulate/simulate][core/simulate][core][simulate][pixel]") {
  // import model
  model::Model s;
  QFile f(":/models/very-simple-model.xml");
  f.open(QIODevice::ReadOnly);
  s.importSBMLString(f.readAll().toStdString());

  // check fields have correct compartments & sizes
  const auto *fa1 = s.getSpecies().getField("A_c1");
  REQUIRE(fa1->getCompartment()->getId() == "c1");
  REQUIRE(fa1->getId() == "A_c1");
  const auto *fb1 = s.getSpecies().getField("B_c1");
  REQUIRE(fb1->getCompartment()->getId() == "c1");
  REQUIRE(fb1->getId() == "B_c1");
  const auto *fa2 = s.getSpecies().getField("A_c2");
  REQUIRE(fa2->getCompartment()->getId() == "c2");
  REQUIRE(fa2->getId() == "A_c2");
  const auto *fb2 = s.getSpecies().getField("B_c2");
  REQUIRE(fb2->getCompartment()->getId() == "c2");
  REQUIRE(fb2->getId() == "B_c2");
  const auto *fa3 = s.getSpecies().getField("A_c3");
  REQUIRE(fa3->getCompartment()->getId() == "c3");
  REQUIRE(fa3->getId() == "A_c3");
  const auto *fb3 = s.getSpecies().getField("B_c3");
  REQUIRE(fb3->getCompartment()->getId() == "c3");
  REQUIRE(fb3->getId() == "B_c3");

  // check membranes have correct compartment pairs & sizes
  const auto &m0 = s.getMembranes().getMembranes()[0];
  REQUIRE(m0.getId() == "c1_c2_membrane");
  REQUIRE(m0.getCompartmentA()->getId() == "c1");
  REQUIRE(m0.getCompartmentB()->getId() == "c2");
  REQUIRE(m0.getIndexPairs().size() == 338);

  const auto &m1 = s.getMembranes().getMembranes()[1];
  REQUIRE(m1.getId() == "c2_c3_membrane");
  REQUIRE(m1.getCompartmentA()->getId() == "c2");
  REQUIRE(m1.getCompartmentB()->getId() == "c3");
  REQUIRE(m1.getIndexPairs().size() == 108);

  // 1st order RK, fixed timestep simulation
  simulate::Options options;
  options.pixel.integrator = simulate::PixelIntegratorType::RK101;
  options.pixel.maxErr = {std::numeric_limits<double>::max(),
                          std::numeric_limits<double>::max()};
  options.pixel.maxTimestep = 0.01;
  options.pixel.enableMultiThreading = false;
  options.pixel.maxThreads = 1;
  simulate::Simulation sim(s, simulate::SimulatorType::Pixel, options);

  // check initial concentrations:
  // note: A_c1 is constant, so not part of simulation
  REQUIRE(sim.getAvgMinMax(0, 0, 0).avg == dbl_approx(0.0));
  REQUIRE(sim.getAvgMinMax(0, 1, 0).avg == dbl_approx(0.0));
  REQUIRE(sim.getAvgMinMax(0, 1, 1).avg == dbl_approx(0.0));
  REQUIRE(sim.getAvgMinMax(0, 2, 0).avg == dbl_approx(0.0));
  REQUIRE(sim.getAvgMinMax(0, 2, 1).avg == dbl_approx(0.0));

  WHEN("one Euler steps: diffusion of A into c2") {
    sim.doTimesteps(0.01);
    REQUIRE(sim.getAvgMinMax(1, 0, 0).avg == dbl_approx(0.0));
    REQUIRE(sim.getAvgMinMax(1, 1, 0).avg > 0);
    REQUIRE(sim.getAvgMinMax(1, 1, 1).avg == dbl_approx(0.0));
    REQUIRE(sim.getAvgMinMax(1, 2, 0).avg == dbl_approx(0.0));
    REQUIRE(sim.getAvgMinMax(1, 2, 1).avg == dbl_approx(0.0));
  }

  WHEN("many Euler steps: all species non-zero") {
    sim.doTimesteps(1.00);
    REQUIRE(sim.getAvgMinMax(1, 0, 0).avg > 0);
    REQUIRE(sim.getAvgMinMax(1, 1, 0).avg > 0);
    REQUIRE(sim.getAvgMinMax(1, 1, 1).avg > 0);
    REQUIRE(sim.getAvgMinMax(1, 2, 0).avg > 0);
    REQUIRE(sim.getAvgMinMax(1, 2, 1).avg > 0);
  }
}

static void rescaleMembraneReacRates(model::Model &s, double factor) {
  for (const auto &memId : s.getMembranes().getIds()) {
    for (const auto &reacId : s.getReactions().getIds(memId)) {
      for (const auto &paramId : s.getReactions().getParameterIds(reacId)) {
        double v{s.getReactions().getParameterValue(reacId, paramId)};
        s.getReactions().setParameterValue(reacId, paramId, v * factor);
      }
    }
  }
}

SCENARIO("Simulate: very_simple_model, change pixel size, Pixel sim",
         "[core/simulate/simulate][core/simulate][core][simulate]") {
  double epsilon{1e-8};
  double margin{1e-13};
  // import model
  model::Model s1;
  QFile f(":/models/very-simple-model.xml");
  f.open(QIODevice::ReadOnly);
  std::string str{f.readAll().toStdString()};
  s1.importSBMLString(str);
  // 1st order RK, fixed timestep simulation
  // pixel width: 1
  // length unit: m
  // volume unit: L
  // alpha = length^3/volume = 1e3
  REQUIRE(s1.getGeometry().getPixelWidth() == dbl_approx(1.0));
  REQUIRE(s1.getUnits().getLength().name == "m");
  REQUIRE(s1.getUnits().getVolume().name == "L");
  simulate::Options options;
  options.pixel.integrator = simulate::PixelIntegratorType::RK101;
  options.pixel.maxErr = {std::numeric_limits<double>::max(),
                          std::numeric_limits<double>::max()};
  options.pixel.maxTimestep = 0.01;
  options.pixel.enableMultiThreading = false;
  options.pixel.maxThreads = 1;
  simulate::Simulation sim(s1, simulate::SimulatorType::Pixel, options);
  sim.doTimesteps(0.20);
  REQUIRE(sim.getTimePoints().size() == 2);

  // 3x pixel width
  {
    model::Model s;
    s.importSBMLString(str);
    s.getGeometry().setPixelWidth(3.0);
    REQUIRE(s.getGeometry().getPixelWidth() == dbl_approx(3.0));
    // pixel width -> 3x larger
    // units, concentrations & reaction rates unchanged
    // but membrane flux per pixel -> 9x larger
    // while volume of pixel -> 27x larger
    // so net conc change from membrane flux in pixel -> 9x/27x = 1/3
    // multiply membrane rate by 3 to compensate for this
    rescaleMembraneReacRates(s, 3.0);
    simulate::Simulation sim2(s, simulate::SimulatorType::Pixel, options);
    sim2.doTimesteps(0.20);
    REQUIRE(sim2.getTimePoints().size() == 2);
    REQUIRE(sim2.errorMessage().empty());
    for (auto iComp : {std::size_t(0), std::size_t(1), std::size_t(2)}) {
      for (auto iSpec : {std::size_t(0), std::size_t(1)}) {
        if (!(iComp == 0 && iSpec == 1)) {
          REQUIRE(sim2.getAvgMinMax(1, iComp, iSpec).avg ==
                  Catch::Approx(sim.getAvgMinMax(1, iComp, iSpec).avg)
                      .epsilon(epsilon)
                      .margin(margin));
        }
      }
    }
  }
  // 0.27* pixel width
  {
    model::Model s;
    s.importSBMLString(str);
    s.getGeometry().setPixelWidth(0.27);
    REQUIRE(s.getGeometry().getPixelWidth() == dbl_approx(0.27));
    // as above
    rescaleMembraneReacRates(s, 0.27);
    simulate::Simulation sim2(s, simulate::SimulatorType::Pixel, options);
    sim2.doTimesteps(0.20);
    REQUIRE(sim2.errorMessage().empty());
    REQUIRE(sim2.getTimePoints().size() == 2);
    for (auto iComp : {std::size_t(0), std::size_t(1), std::size_t(2)}) {
      for (auto iSpec : {std::size_t(0), std::size_t(1)}) {
        if (!(iComp == 0 && iSpec == 1)) {
          REQUIRE(sim2.getAvgMinMax(1, iComp, iSpec).avg ==
                  Catch::Approx(sim.getAvgMinMax(1, iComp, iSpec).avg)
                      .epsilon(epsilon)
                      .margin(margin));
        }
      }
    }
  }
}

SCENARIO("Simulate: very_simple_model, membrane reaction units consistency",
         "[core/simulate/simulate][core/simulate][core][simulate]") {
  for (auto simulatorType :
       {simulate::SimulatorType::DUNE, simulate::SimulatorType::Pixel}) {
    double epsilon{1e-8};
    double margin{1e-13};
    if (simulatorType == simulate::SimulatorType::DUNE) {
      epsilon = 1e-7;
    }
    // import model
    model::Model s1;
    QFile f(":/models/very-simple-model.xml");
    f.open(QIODevice::ReadOnly);
    std::string str{f.readAll().toStdString()};
    s1.importSBMLString(str);
    // 1st order RK, fixed timestep simulation
    // pixel width: 1
    // length unit: m
    // volume unit: L
    // alpha = length^3/volume = 1e3
    REQUIRE(s1.getGeometry().getPixelWidth() == dbl_approx(1.0));
    REQUIRE(s1.getUnits().getLength().name == "m");
    REQUIRE(s1.getUnits().getVolume().name == "L");
    simulate::Options options;
    options.pixel.integrator = simulate::PixelIntegratorType::RK101;
    options.pixel.maxErr = {std::numeric_limits<double>::max(),
                            std::numeric_limits<double>::max()};
    options.pixel.maxTimestep = 0.01;
    options.pixel.enableMultiThreading = false;
    options.pixel.maxThreads = 1;
    options.dune.dt = 0.01;
    options.dune.minDt = 0.001;
    options.dune.maxDt = 0.011;
    CAPTURE(simulatorType);
    simulate::Simulation sim(s1, simulatorType, options);
    sim.doTimesteps(0.20);
    REQUIRE(sim.getTimePoints().size() == 2);

    // Length unit -> 10x smaller
    {
      model::Model s;
      s.importSBMLString(str);
      s.getUnits().setLengthIndex(1);
      // concentrations & compartment reaction rates unaffected by length units
      // membrane reaction in units of [amount]/[length]^3 is the same
      // but since [volume] is unchanged and [length] is 10x smaller,
      // the concentration in [amount]/[volume] is 1000x larger
      // divide membrane reaction rate by 1000 to compensate for this
      rescaleMembraneReacRates(s, 1e-3);
      s.getUnits().setLengthIndex(1);
      REQUIRE(s.getUnits().getLength().name == "dm");
      simulate::Simulation sim2(s, simulatorType, options);
      sim2.doTimesteps(0.20);
      REQUIRE(sim2.getTimePoints().size() == 2);
      REQUIRE(sim2.errorMessage().empty());
      for (auto iComp : {std::size_t(0), std::size_t(1), std::size_t(2)}) {
        for (auto iSpec : {std::size_t(0), std::size_t(1)}) {
          if (!(iComp == 0 && iSpec == 1)) {
            REQUIRE(sim2.getAvgMinMax(1, iComp, iSpec).avg ==
                    Catch::Approx(sim.getAvgMinMax(1, iComp, iSpec).avg)
                        .epsilon(epsilon)
                        .margin(margin));
          }
        }
      }
    }

    // Length unit -> 100x smaller
    {
      // as above
      model::Model s;
      s.importSBMLString(str);
      rescaleMembraneReacRates(s, 1e-6);
      s.getUnits().setLengthIndex(2);
      REQUIRE(s.getUnits().getLength().name == "cm");
      simulate::Simulation sim2(s, simulatorType, options);
      sim2.doTimesteps(0.20);
      REQUIRE(sim2.getTimePoints().size() == 2);
      REQUIRE(sim2.errorMessage().empty());
      for (auto iComp : {std::size_t(0), std::size_t(1), std::size_t(2)}) {
        for (auto iSpec : {std::size_t(0), std::size_t(1)}) {
          if (!(iComp == 0 && iSpec == 1)) {
            REQUIRE(sim2.getAvgMinMax(1, iComp, iSpec).avg ==
                    Catch::Approx(sim.getAvgMinMax(1, iComp, iSpec).avg)
                        .epsilon(epsilon)
                        .margin(margin));
          }
        }
      }
    }
    // Volume unit -> 10x smaller
    {
      // concentration values unchanged, but [volume] 10x smaller
      // i.e. concentrations are 10x higher in [amount]/[length]^3 units
      // so membrane rates need to be 10x higher to compensate
      model::Model s;
      s.importSBMLString(str);
      rescaleMembraneReacRates(s, 10);
      s.getUnits().setVolumeIndex(1);
      REQUIRE(s.getUnits().getVolume().name == "dL");
      simulate::Simulation sim2(s, simulatorType, options);
      sim2.doTimesteps(0.20);
      REQUIRE(sim2.getTimePoints().size() == 2);
      REQUIRE(sim2.errorMessage().empty());
      for (auto iComp : {std::size_t(0), std::size_t(1), std::size_t(2)}) {
        for (auto iSpec : {std::size_t(0), std::size_t(1)}) {
          if (!(iComp == 0 && iSpec == 1)) {
            REQUIRE(sim2.getAvgMinMax(1, iComp, iSpec).avg ==
                    Catch::Approx(sim.getAvgMinMax(1, iComp, iSpec).avg)
                        .epsilon(epsilon)
                        .margin(margin));
          }
        }
      }
    }
    // Volume unit -> 1000x smaller
    {
      // as above
      model::Model s;
      s.importSBMLString(str);
      rescaleMembraneReacRates(s, 1000);
      s.getUnits().setVolumeIndex(3);
      REQUIRE(s.getUnits().getVolume().name == "mL");
      simulate::Simulation sim2(s, simulatorType, options);
      sim2.doTimesteps(0.20);
      REQUIRE(sim2.getTimePoints().size() == 2);
      REQUIRE(sim2.errorMessage().empty());
      for (auto iComp : {std::size_t(0), std::size_t(1), std::size_t(2)}) {
        for (auto iSpec : {std::size_t(0), std::size_t(1)}) {
          if (!(iComp == 0 && iSpec == 1)) {
            REQUIRE(sim2.getAvgMinMax(1, iComp, iSpec).avg ==
                    Catch::Approx(sim.getAvgMinMax(1, iComp, iSpec).avg)
                        .epsilon(epsilon)
                        .margin(margin));
          }
        }
      }
    }
    // Volume unit -> 1000x larger (L -> m^3)
    {
      model::Model s;
      s.importSBMLString(str);
      rescaleMembraneReacRates(s, 1e-3);
      s.getUnits().setVolumeIndex(4);
      REQUIRE(s.getUnits().getVolume().name == "m3");
      simulate::Simulation sim2(s, simulatorType, options);
      sim2.doTimesteps(0.20);
      REQUIRE(sim2.getTimePoints().size() == 2);
      REQUIRE(sim2.errorMessage().empty());
      for (auto iComp : {std::size_t(0), std::size_t(1), std::size_t(2)}) {
        for (auto iSpec : {std::size_t(0), std::size_t(1)}) {
          if (!(iComp == 0 && iSpec == 1)) {
            REQUIRE(sim2.getAvgMinMax(1, iComp, iSpec).avg ==
                    Catch::Approx(sim.getAvgMinMax(1, iComp, iSpec).avg)
                        .epsilon(epsilon)
                        .margin(margin));
          }
        }
      }
    }
    // Volume unit equivalent (L -> dm^3)
    {
      model::Model s;
      s.importSBMLString(str);
      s.getUnits().setVolumeIndex(5);
      REQUIRE(s.getUnits().getVolume().name == "dm3");
      simulate::Simulation sim2(s, simulatorType, options);
      sim2.doTimesteps(0.20);
      REQUIRE(sim2.getTimePoints().size() == 2);
      REQUIRE(sim2.errorMessage().empty());
      for (auto iComp : {std::size_t(0), std::size_t(1), std::size_t(2)}) {
        for (auto iSpec : {std::size_t(0), std::size_t(1)}) {
          if (!(iComp == 0 && iSpec == 1)) {
            REQUIRE(sim2.getAvgMinMax(1, iComp, iSpec).avg ==
                    Catch::Approx(sim.getAvgMinMax(1, iComp, iSpec).avg)
                        .epsilon(epsilon)
                        .margin(margin));
          }
        }
      }
    }
  }
}

// return r2 distance from origin of point
static double r2(const QPoint &p) {
  // nb: flip y axis since qpoint has y=0 at top
  // use point at centre of pixel
  return std::pow(p.x() - 48 + 0.5, 2) + std::pow((99 - p.y()) - 48 + 0.5, 2);
}

// return analytic prediction for concentration
// u(t) = [t0/(t+t0)] * exp(-r^2/(4Dt))
static double analytic(const QPoint &p, double t, double D, double t0) {
  return (t0 / (t + t0)) * exp(-r2(p) / (4.0 * D * (t + t0)));
}

SCENARIO(
    "Simulate: single-compartment-diffusion, circular geometry",
    "[core/simulate/simulate][core/simulate][core][simulate][dune][pixel][Q]") {
  // see docs/tests/diffusion.rst for analytic expressions used here
  // NB central point of initial distribution: (48,99-48) <-> ix=1577

  constexpr double pi = 3.14159265358979323846;
  double sigma2 = 36.0;
  double epsilon = 1e-10;
  // import model
  model::Model s;
  if (QFile f(":/models/single-compartment-diffusion.xml");
      f.open(QIODevice::ReadOnly)) {
    s.importSBMLString(f.readAll().toStdString());
  }

  // check fields have correct compartments
  const auto *slow = s.getSpecies().getField("slow");
  REQUIRE(slow->getCompartment()->getId() == "circle");
  REQUIRE(slow->getId() == "slow");
  const auto *fast = s.getSpecies().getField("fast");
  REQUIRE(fast->getCompartment()->getId() == "circle");
  REQUIRE(fast->getId() == "fast");

  // check total initial concentration matches analytic value
  double analytic_total = sigma2 * pi;
  for (const auto &c : {slow->getConcentration(), fast->getConcentration()}) {
    REQUIRE(std::abs(utils::sum(c) - analytic_total) / analytic_total <
            epsilon);
  }

  // check initial distribution matches analytic one
  for (const auto &f : {slow, fast}) {
    double D = f->getDiffusionConstant();
    double t0 = sigma2 / 4.0 / D;
    double maxRelErr = 0;
    for (std::size_t i = 0; i < f->getCompartment()->nPixels(); ++i) {
      const auto &p = f->getCompartment()->getPixel(i);
      double c = analytic(p, 0, D, t0);
      double relErr = std::abs(f->getConcentration()[i] - c) / c;
      maxRelErr = std::max(maxRelErr, relErr);
    }
    CAPTURE(f->getDiffusionConstant());
    REQUIRE(maxRelErr < epsilon);
  }

  simulate::Options options;
  options.pixel.maxErr = {std::numeric_limits<double>::max(), 0.01};
  options.dune.dt = 1.0;
  options.dune.maxDt = 1.0;
  options.dune.minDt = 0.5;
  for (auto simType :
       {simulate::SimulatorType::Pixel, simulate::SimulatorType::DUNE}) {
    // relative error on integral of initial concentration over all pixels:
    double initialRelativeError{1e-9};
    // largest relative error of any pixel after simulation:
    double evolvedMaxRelativeError{0.02};
    // average of relative errors of all pixels after simulation:
    double evolvedAvgRelativeError{0.003};
    if (simType == simulate::SimulatorType::DUNE) {
      // increase allowed error for dune simulation
      initialRelativeError = 0.02;
      evolvedMaxRelativeError = 0.3;
      evolvedAvgRelativeError = 0.10;
    }

    // integrate & compare
    simulate::Simulation sim(s, simType, options);
    double t = 10.0;
    for (std::size_t step = 0; step < 2; ++step) {
      sim.doTimesteps(t);
      for (auto speciesIndex : {std::size_t{0}, std::size_t{1}}) {
        // check total concentration is conserved
        auto c = sim.getConc(step + 1, 0, speciesIndex);
        double totalC = utils::sum(c);
        double relErr = std::abs(totalC - analytic_total) / analytic_total;
        CAPTURE(simType);
        CAPTURE(speciesIndex);
        CAPTURE(sim.getTimePoints().back());
        CAPTURE(totalC);
        CAPTURE(analytic_total);
        REQUIRE(relErr < initialRelativeError);
      }
    }

    // check new distribution matches analytic one
    std::vector<double> D{slow->getDiffusionConstant(),
                          fast->getDiffusionConstant()};
    std::size_t timeIndex = sim.getTimePoints().size() - 1;
    t = sim.getTimePoints().back();
    for (auto speciesIndex : {std::size_t{0}, std::size_t{1}}) {
      double t0 = sigma2 / 4.0 / D[speciesIndex];
      auto conc = sim.getConc(timeIndex, 0, speciesIndex);
      double maxRelErr{0};
      double avgRelErr{0};
      std::size_t count{0};
      for (std::size_t i = 0; i < slow->getCompartment()->nPixels(); ++i) {
        const auto &p = slow->getCompartment()->getPixel(i);
        // only check part within a radius of 16 units from centre to avoid
        // boundary effects: analytic solution is in infinite volume
        if (r2(p) < 16 * 16) {
          double c_analytic = analytic(p, t, D[speciesIndex], t0);
          double relErr = std::abs(conc[i] - c_analytic) / c_analytic;
          avgRelErr += relErr;
          ++count;
          maxRelErr = std::max(maxRelErr, relErr);
        }
      }
      avgRelErr /= static_cast<double>(count);
      CAPTURE(simType);
      CAPTURE(t);
      REQUIRE(maxRelErr < evolvedMaxRelativeError);
      REQUIRE(avgRelErr < evolvedAvgRelativeError);
    }
  }
}

SCENARIO(
    "Simulate: small-single-compartment-diffusion, circular geometry",
    "[core/simulate/simulate][core/simulate][core][simulate][dune][pixel]") {
  WHEN("many steps: both species end up equally & uniformly distributed") {
    double epsilon = 1e-3;
    // import model
    model::Model s;
    if (QFile f(":/models/small-single-compartment-diffusion.xml");
        f.open(QIODevice::ReadOnly)) {
      s.importSBMLString(f.readAll().toStdString());
    }
    simulate::Options options;
    options.pixel.maxErr = {std::numeric_limits<double>::max(), 0.001};
    options.pixel.maxThreads = 2;
    options.dune.dt = 0.5;
    for (auto simulator :
         {simulate::SimulatorType::DUNE, simulate::SimulatorType::Pixel}) {
      auto sim = simulate::Simulation(s, simulator, options);
      REQUIRE(sim.getIsRunning() == false);
      REQUIRE(sim.getIsStopping() == false);
      REQUIRE(sim.getNCompletedTimesteps() == 1);
      // run simulation in another thread
      auto simSteps =
          std::async(std::launch::async, &simulate::Simulation::doTimesteps,
                     &sim, 50.0, 1);
      // this .get() blocks until simulation is finished
      REQUIRE(simSteps.get() >= 0);
      REQUIRE(sim.getIsRunning() == false);
      REQUIRE(sim.getIsStopping() == false);
      REQUIRE(sim.getNCompletedTimesteps() == 2);
      auto timeIndex = sim.getTimePoints().size() - 1;
      // after many steps in a finite volume, diffusion has reached the limiting
      // case of a uniform distribution
      std::size_t speciesIndex = 0;
      for (const auto &species : {"slow", "fast"}) {
        auto conc = sim.getAvgMinMax(timeIndex, 0, speciesIndex++);
        CAPTURE(simulator);
        CAPTURE(species);
        CAPTURE(conc.min);
        CAPTURE(conc.avg);
        CAPTURE(conc.max);
        REQUIRE(std::abs((conc.min - conc.avg) / conc.avg) < epsilon);
        REQUIRE(std::abs((conc.max - conc.avg) / conc.avg) < epsilon);
      }
    }
  }
}

SCENARIO("Pixel simulator: brusselator model, RK2, RK3, RK4",
         "[core/simulate/simulate][core/simulate][core][simulate][pixel]") {
  double eps{1e-20};
  double time{30.0};
  double maxAllowedRelErr{0.01};
  // import model
  model::Model s;
  if (QFile f(":/models/brusselator-model.xml"); f.open(QIODevice::ReadOnly)) {
    s.importSBMLString(f.readAll().toStdString());
  }
  // do accurate simulation
  simulate::Options options;
  options.pixel.maxErr = {std::numeric_limits<double>::max(), 1e-6};
  options.pixel.maxThreads = 2;
  options.pixel.integrator = simulate::PixelIntegratorType::RK435;
  simulate::Simulation sim(s, simulate::SimulatorType::Pixel, options);
  sim.doTimesteps(time);
  auto c4_accurate = sim.getConc(sim.getTimePoints().size() - 1, 0, 0);
  // check lower accuracy & different orders are consistent
  for (bool multithreaded : {false, true}) {
    for (auto integrator : {simulate::PixelIntegratorType::RK212,
                            simulate::PixelIntegratorType::RK323,
                            simulate::PixelIntegratorType::RK435}) {
      double maxRelDiff = 0;
      options.pixel.integrator = integrator;
      options.pixel.enableMultiThreading = multithreaded;
      options.pixel.maxErr = {std::numeric_limits<double>::max(),
                              maxAllowedRelErr};
      simulate::Simulation sim2(s, simulate::SimulatorType::Pixel, options);
      sim2.doTimesteps(time);
      auto conc = sim2.getConc(sim.getTimePoints().size() - 1, 0, 0);
      for (std::size_t i = 0; i < conc.size(); ++i) {
        maxRelDiff = std::max(maxRelDiff, (conc[i] - c4_accurate[i]) /
                                              (c4_accurate[i] + eps));
      }
      CAPTURE(multithreaded);
      CAPTURE(integrator);
      REQUIRE(maxRelDiff < maxAllowedRelErr);
    }
  }
}

SCENARIO("DUNE: simulation",
         "[core/simulate/simulate][core/simulate][core][simulate][dune]") {
  GIVEN("ABtoC model") {
    model::Model s;
    if (QFile f(":/models/ABtoC.xml"); f.open(QIODevice::ReadOnly)) {
      s.importSBMLString(f.readAll().toStdString());
    }

    // set spatially constant initial conditions
    s.getSpecies().setInitialConcentration("A", 1.0);
    s.getSpecies().setInitialConcentration("B", 1.0);
    s.getSpecies().setInitialConcentration("C", 0.0);

    simulate::Options options;
    options.dune.dt = 0.01;
    options.dune.maxDt = 0.01;
    options.dune.minDt = 0.005;
    options.dune.integrator = "alexander_2";
    simulate::Simulation duneSim(s, simulate::SimulatorType::DUNE, options);
    REQUIRE(duneSim.getAvgMinMax(0, 0, 0).avg == dbl_approx(1.0));
    REQUIRE(duneSim.getAvgMinMax(0, 0, 1).avg == dbl_approx(1.0));
    REQUIRE(duneSim.getAvgMinMax(0, 0, 2).avg == dbl_approx(0.0));
    duneSim.doTimesteps(0.05);
    CAPTURE(duneSim.getTimePoints().size());
    auto timeIndex = duneSim.getTimePoints().size() - 1;
    auto imgConc = duneSim.getConcImage(timeIndex);
    REQUIRE(std::abs(duneSim.getAvgMinMax(timeIndex, 0, 0).avg - 0.995) < 1e-4);
    REQUIRE(std::abs(duneSim.getAvgMinMax(timeIndex, 0, 1).avg - 0.995) < 1e-4);
    REQUIRE(std::abs(duneSim.getAvgMinMax(timeIndex, 0, 2).avg - 0.005) < 1e-4);
    REQUIRE(imgConc.size() == QSize(100, 100));
  }
  GIVEN("very-simple-model") {
    model::Model s;
    if (QFile f(":/models/very-simple-model.xml");
        f.open(QIODevice::ReadOnly)) {
      s.importSBMLString(f.readAll().toStdString());
    }
    simulate::Options options;
    options.dune.dt = 0.01;
    simulate::Simulation duneSim(s, simulate::SimulatorType::DUNE, options);
    duneSim.doTimesteps(0.01);
    REQUIRE(duneSim.errorMessage().empty());
  }
}

SCENARIO("getConcImage",
         "[core/simulate/simulate][core/simulate][core][simulate]") {
  GIVEN("very-simple-model") {
    model::Model s;
    if (QFile f(":/models/very-simple-model.xml");
        f.open(QIODevice::ReadOnly)) {
      s.importSBMLString(f.readAll().toStdString());
    }
    simulate::Options options;
    simulate::Simulation sim(s, simulate::SimulatorType::Pixel, options);
    sim.doTimesteps(0.5);
    sim.doTimesteps(0.5);
    REQUIRE(sim.errorMessage().empty());

    // draw no species, any normalisation
    for (auto allTime : {true, false}) {
      for (auto allSpecies : {true, false}) {
        auto img0{sim.getConcImage(1, {{}, {}, {}}, allTime, allSpecies)};
        img0.save("concImg0.png");
        REQUIRE(img0.pixel(48, 43) == qRgb(0, 0, 0));
        REQUIRE(img0.pixel(49, 43) == qRgb(0, 0, 0));
        REQUIRE(img0.pixel(33, 8) == qRgb(0, 0, 0));
      }
    }

    // draw B_out species only
    for (auto allSpecies : {true, false}) {
      // this timepoint
      auto img1a{sim.getConcImage(1, {{0}, {}, {}}, false, allSpecies)};
      img1a.save("concImg1a.png");
      REQUIRE(img1a.pixel(48, 43) == qRgb(0, 0, 0));
      REQUIRE(img1a.pixel(49, 43) == qRgb(0, 0, 0));
      REQUIRE(img1a.pixel(33, 8) == qRgb(0, 0, 0));
      REQUIRE(img1a.pixel(59, 33) == qRgb(145, 30, 180));

      // all timepoints
      auto img1b{sim.getConcImage(1, {{0}, {}, {}}, true, allSpecies)};
      img1b.save("concImg1b.png");
      REQUIRE(img1b.pixel(48, 43) == qRgb(0, 0, 0));
      REQUIRE(img1b.pixel(49, 43) == qRgb(0, 0, 0));
      REQUIRE(img1b.pixel(33, 8) == qRgb(0, 0, 0));
      REQUIRE(img1b.pixel(33, 8) == qRgb(0, 0, 0));
    }

    // draw all species, normalise to max of each species, at this timepoint
    auto img1{sim.getConcImage(1, {}, false, false)};
    img1.save("concImg1.png");
    REQUIRE(img1.pixel(48, 43) == qRgb(255, 255, 225));
    REQUIRE(img1.pixel(49, 43) == qRgb(245, 130, 48));
    REQUIRE(img1.pixel(33, 8) == qRgb(47, 142, 59));

    // draw all species, normalise to max of all species, at this timepoint
    auto img3{sim.getConcImage(1, {}, false, true)};
    img3.save("concImg3.png");
    REQUIRE(img3.pixel(48, 43) == qRgb(0, 0, 0));
    REQUIRE(img3.pixel(49, 43) == qRgb(0, 0, 0));
    REQUIRE(img3.pixel(33, 8) == qRgb(47, 142, 59));

    // draw all species, normalise to max of each/all species, at all timepoints
    for (auto allSpecies : {true, false}) {
      auto img2{sim.getConcImage(1, {}, true, allSpecies)};
      img2.save("concImg2.png");
      REQUIRE(img2.pixel(48, 43) == qRgb(0, 0, 0));
      REQUIRE(img2.pixel(49, 43) == qRgb(0, 0, 0));
      REQUIRE(img2.pixel(33, 8) == qRgb(31, 93, 39));
    }
  }
}

SCENARIO("PyConc",
         "[core/simulate/simulate][core/simulate][core][simulate][python]") {
  GIVEN("ABtoC model") {
    model::Model s;
    if (QFile f(":/models/ABtoC.xml"); f.open(QIODevice::ReadOnly)) {
      s.importSBMLString(f.readAll().toStdString());
    }
    simulate::Simulation sim(s);
    auto pyConcs = sim.getPyConcs(0);
    REQUIRE(pyConcs.size() == 3);
    const auto &cA = pyConcs["A"];
    REQUIRE(cA.size() == 100);
    REQUIRE(cA[0].size() == 100);
    REQUIRE(cA[0][0] == dbl_approx(0.0));
  }
}

SCENARIO("Reactions depend on x, y, t",
         "[core/simulate/simulate][core/simulate][core][simulate]") {
  model::Model s;
  QFile f(":/test/models/txy.xml");
  f.open(QIODevice::ReadOnly);
  s.importSBMLString(f.readAll().toStdString());
  constexpr double eps{1e-20};
  constexpr double dt{1e-3};
  simulate::Options options;
  options.dune.dt = dt;
  options.pixel.integrator = simulate::PixelIntegratorType::RK101;
  options.pixel.maxErr = {std::numeric_limits<double>::max(),
                          std::numeric_limits<double>::max()};
  options.pixel.maxTimestep = dt;
  GIVEN("reaction with t-dependence") {
    // fairly tight tolerance, as solution is spatially uniform, so mesh vs
    // pixel geometry is not a factor when comparing Pixel and Dune here
    constexpr double maxAllowedAbsDiff{1e-10};
    constexpr double maxAllowedRelDiff{1e-7};
    s.getSpecies().remove("A");
    s.getSpecies().remove("B");
    simulate::Simulation simPixel{s, simulate::SimulatorType::Pixel};
    simPixel.doTimesteps(dt, 1);
    REQUIRE(simPixel.errorMessage().empty());
    REQUIRE(simPixel.getNCompletedTimesteps() == 2);
    simulate::Simulation simDune{s, simulate::SimulatorType::DUNE};
    simDune.doTimesteps(dt, 1);
    REQUIRE(simDune.errorMessage().empty());
    REQUIRE(simDune.getNCompletedTimesteps() == 2);
    auto p{simPixel.getConc(1, 0, 0)};
    auto d{simDune.getConc(1, 0, 0)};
    REQUIRE(p.size() == d.size());
    double maxAbsDiff{0};
    double maxRelDiff{0};
    for (std::size_t i = 0; i < p.size(); ++i) {
      maxAbsDiff = std::max(maxAbsDiff, std::abs(p[i] - d[i]));
      maxRelDiff = std::max(maxRelDiff, std::abs(p[i] - d[i]) /
                                            (std::abs(p[i] + d[i] + eps)));
    }
    CAPTURE(maxAbsDiff);
    CAPTURE(maxRelDiff);
    REQUIRE(maxAbsDiff < maxAllowedAbsDiff);
    REQUIRE(maxRelDiff < maxAllowedRelDiff);
  }
  GIVEN("reaction with x,y-dependence") {
    // looser tolerance: mesh distorts results
    constexpr double maxAllowedAbsDiff{0.002};
    constexpr double maxAllowedRelDiff{0.03};
    s.getSpecies().remove("C");
    simulate::Simulation simPixel{s, simulate::SimulatorType::Pixel};
    simPixel.doTimesteps(dt, 1);
    REQUIRE(simPixel.errorMessage().empty());
    REQUIRE(simPixel.getNCompletedTimesteps() == 2);
    simulate::Simulation simDune{s, simulate::SimulatorType::DUNE};
    simDune.doTimesteps(dt, 1);
    REQUIRE(simDune.errorMessage().empty());
    REQUIRE(simDune.getNCompletedTimesteps() == 2);
    for (std::size_t speciesIndex = 0; speciesIndex < 2; ++speciesIndex) {
      auto p{simPixel.getConc(1, 0, speciesIndex)};
      auto d{simDune.getConc(1, 0, speciesIndex)};
      REQUIRE(p.size() == d.size());
      double maxAbsDiff{0};
      double maxRelDiff{0};
      for (std::size_t i = 0; i < p.size(); ++i) {
        maxAbsDiff = std::max(maxAbsDiff, std::abs(p[i] - d[i]));
        maxRelDiff = std::max(maxRelDiff, std::abs(p[i] - d[i]) /
                                              (std::abs(p[i] + d[i] + eps)));
      }
      CAPTURE(speciesIndex);
      CAPTURE(maxAbsDiff);
      CAPTURE(maxRelDiff);
      REQUIRE(maxAbsDiff < maxAllowedAbsDiff);
      REQUIRE(maxRelDiff < maxAllowedRelDiff);
    }
  }
  GIVEN("reaction with t,x,y-dependence") {
    // looser tolerance: mesh distorts results
    constexpr double maxAllowedAbsDiff{0.002};
    constexpr double maxAllowedRelDiff{0.03};
    simulate::Simulation simPixel{s, simulate::SimulatorType::Pixel};
    simPixel.doTimesteps(dt, 1);
    REQUIRE(simPixel.errorMessage().empty());
    REQUIRE(simPixel.getNCompletedTimesteps() == 2);
    simulate::Simulation simDune{s, simulate::SimulatorType::DUNE};
    simDune.doTimesteps(dt, 1);
    REQUIRE(simDune.errorMessage().empty());
    REQUIRE(simDune.getNCompletedTimesteps() == 2);
    for (std::size_t speciesIndex = 0; speciesIndex < 3; ++speciesIndex) {
      auto p{simPixel.getConc(1, 0, speciesIndex)};
      auto d{simDune.getConc(1, 0, speciesIndex)};
      REQUIRE(p.size() == d.size());
      double maxAbsDiff{0};
      double maxRelDiff{0};
      for (std::size_t i = 0; i < p.size(); ++i) {
        maxAbsDiff = std::max(maxAbsDiff, std::abs(p[i] - d[i]));
        maxRelDiff = std::max(maxRelDiff, std::abs(p[i] - d[i]) /
                                              (std::abs(p[i] + d[i] + eps)));
      }
      CAPTURE(speciesIndex);
      CAPTURE(maxAbsDiff);
      CAPTURE(maxRelDiff);
      REQUIRE(maxAbsDiff < maxAllowedAbsDiff);
      REQUIRE(maxRelDiff < maxAllowedRelDiff);
    }
  }
}

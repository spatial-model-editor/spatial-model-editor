#include "catch_wrapper.hpp"
#include "model.hpp"
#include "sbml_test_data/very_simple_model.hpp"
#include "simulate.hpp"
#include "utils.hpp"
#include <QFile>
#include <algorithm>
#include <cmath>
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
    auto steps = sim.doTimestep(dt);
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
    auto steps = sim.doTimestep(dt);
    REQUIRE(steps == 1);
    double A_c2 = sim.getAvgMinMax(1, 1, 0).avg;
    steps = sim.doTimestep(dt);
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
    sim.doTimestep(dt);
    sim.doTimestep(dt);
    double A_c2 = sim.getAvgMinMax(2, 1, 0).avg;
    double A_c3 = sim.getAvgMinMax(2, 2, 0).avg;
    sim.doTimestep(dt);
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
    sim2.doTimestep(1000);
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
    sim2.doTimestep(eps);
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
    sim.doTimestep(0.01);
    REQUIRE(sim.getAvgMinMax(1, 0, 0).avg == dbl_approx(0.0));
    REQUIRE(sim.getAvgMinMax(1, 1, 0).avg > 0);
    REQUIRE(sim.getAvgMinMax(1, 1, 1).avg == dbl_approx(0.0));
    REQUIRE(sim.getAvgMinMax(1, 2, 0).avg == dbl_approx(0.0));
    REQUIRE(sim.getAvgMinMax(1, 2, 1).avg == dbl_approx(0.0));
  }

  WHEN("many Euler steps: all species non-zero") {
    sim.doTimestep(1.00);
    REQUIRE(sim.getAvgMinMax(1, 0, 0).avg > 0);
    REQUIRE(sim.getAvgMinMax(1, 1, 0).avg > 0);
    REQUIRE(sim.getAvgMinMax(1, 1, 1).avg > 0);
    REQUIRE(sim.getAvgMinMax(1, 2, 0).avg > 0);
    REQUIRE(sim.getAvgMinMax(1, 2, 1).avg > 0);
  }
}

// return r2 distance from origin of point
static double r2(const QPoint &p) {
  // nb: flip y axis since qpoint has y=0 at top
  return std::pow(p.x() - 48, 2) + std::pow((99 - p.y()) - 48, 2);
}

// return analytic prediction for concentration
// u(t) = [t0/(t+t0)] * exp(-r^2/(4Dt))
static double analytic(const QPoint &p, double t, double D, double t0) {
  return (t0 / (t + t0)) * exp(-r2(p) / (4.0 * D * (t + t0)));
}

SCENARIO(
    "Simulate: single-compartment-diffusion, circular geometry",
    "[core/simulate/simulate][core/simulate][core][simulate][dune][pixel]") {
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
    double initialRelativeError = 1e-9;
    double evolvedRelativeError = 0.02;
    if (simType == simulate::SimulatorType::DUNE) {
      initialRelativeError = 0.05;
      evolvedRelativeError = 0.5;
    }

    // integrate & compare
    simulate::Simulation sim(s, simType, options);
    double t = 10.0;
    for (std::size_t step = 0; step < 2; ++step) {
      sim.doTimestep(t);
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
      double maxRelErr = 0;
      for (std::size_t i = 0; i < slow->getCompartment()->nPixels(); ++i) {
        const auto &p = slow->getCompartment()->getPixel(i);
        // only check part within a radius of 16 units from centre to avoid
        // boundary effects: analytic solution is in infinite volume
        if (r2(p) < 16 * 16) {
          double c_analytic = analytic(p, t, D[speciesIndex], t0);
          double relErr = std::abs(conc[i] - c_analytic) / c_analytic;
          maxRelErr = std::max(maxRelErr, relErr);
        }
      }
      CAPTURE(simType);
      CAPTURE(t);
      REQUIRE(maxRelErr < evolvedRelativeError);
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
      sim.doTimestep(50.0);
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
  double eps = 1e-20;
  double time = 30.0;
  double relErr = 0.01;
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
  sim.doTimestep(time);
  auto c4_accurate = sim.getConc(sim.getTimePoints().size() - 1, 0, 0);
  // check lower accuracy & different orders are consistent
  for (bool multithreaded : {false, true}) {
    for (auto integrator : {simulate::PixelIntegratorType::RK212,
                            simulate::PixelIntegratorType::RK323,
                            simulate::PixelIntegratorType::RK435}) {
      double maxRelDiff = 0;
      options.pixel.integrator = integrator;
      options.pixel.enableMultiThreading = multithreaded;
      options.pixel.maxErr = {std::numeric_limits<double>::max(), relErr};
      simulate::Simulation sim2(s, simulate::SimulatorType::Pixel, options);
      sim2.doTimestep(time);
      auto conc = sim2.getConc(sim.getTimePoints().size() - 1, 0, 0);
      for (std::size_t i = 0; i < conc.size(); ++i) {
        maxRelDiff = std::max(maxRelDiff, (conc[i] - c4_accurate[i]) /
                                              (c4_accurate[i] + eps));
      }
      CAPTURE(multithreaded);
      CAPTURE(integrator);
      REQUIRE(maxRelDiff < relErr);
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
    duneSim.doTimestep(0.05);
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
    duneSim.doTimestep(0.01);
    REQUIRE(duneSim.errorMessage().empty());
  }
}

SCENARIO("getConcImage",
         "[core/simulate/simulate][core/simulate][core][simulate][QQ]") {
  GIVEN("very-simple-model") {
    model::Model s;
    if (QFile f(":/models/very-simple-model.xml");
        f.open(QIODevice::ReadOnly)) {
      s.importSBMLString(f.readAll().toStdString());
    }
    simulate::Options options;
    simulate::Simulation sim(s, simulate::SimulatorType::Pixel, options);
    sim.doTimestep(0.5);
    sim.doTimestep(0.5);
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

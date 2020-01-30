#include <sbml/SBMLDocument.h>
#include <sbml/SBMLReader.h>
#include <sbml/SBMLWriter.h>

#include <QFile>
#include <algorithm>

#include "catch_wrapper.hpp"
#include "sbml.hpp"
#include "sbml_test_data/very_simple_model.hpp"
#include "simulate.hpp"
#include "utils.hpp"

SCENARIO("Simulate: very_simple_model, single pixel geometry",
         "[core][simulate][pixel]") {
  // import model
  std::unique_ptr<libsbml::SBMLDocument> doc(
      libsbml::readSBMLFromString(sbml_test_data::very_simple_model().xml));
  libsbml::SBMLWriter().writeSBML(doc.get(), "tmp.xml");
  sbml::SbmlDocWrapper s;
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
  s.importGeometryFromImage("tmp.bmp");
  s.setPixelWidth(1.0);
  s.setCompartmentColour("c1", col1);
  s.setCompartmentColour("c2", col2);
  s.setCompartmentColour("c3", col3);

  // check we have identified the compartments and membranes
  REQUIRE(s.compartments == QStringList{"c1", "c2", "c3"});
  REQUIRE(s.membranes == QStringList{"c1_c2", "c2_c3"});

  // check fields have correct compartments & sizes
  geometry::Field &fa1 = s.mapSpeciesIdToField.at("A_c1");
  REQUIRE(fa1.geometry->getId() == "c1");
  REQUIRE(fa1.speciesID == "A_c1");
  geometry::Field &fb1 = s.mapSpeciesIdToField.at("B_c1");
  REQUIRE(fb1.geometry->getId() == "c1");
  REQUIRE(fb1.speciesID == "B_c1");
  geometry::Field &fa2 = s.mapSpeciesIdToField.at("A_c2");
  REQUIRE(fa2.geometry->getId() == "c2");
  REQUIRE(fa2.speciesID == "A_c2");
  geometry::Field &fb2 = s.mapSpeciesIdToField.at("B_c2");
  REQUIRE(fb2.geometry->getId() == "c2");
  REQUIRE(fb2.speciesID == "B_c2");
  geometry::Field &fa3 = s.mapSpeciesIdToField.at("A_c3");
  REQUIRE(fa3.geometry->getId() == "c3");
  REQUIRE(fa3.speciesID == "A_c3");
  geometry::Field &fb3 = s.mapSpeciesIdToField.at("B_c3");
  REQUIRE(fb3.geometry->getId() == "c3");
  REQUIRE(fb3.speciesID == "B_c3");

  // check membranes have correct compartment pairs & sizes
  geometry::Membrane &m0 = s.membraneVec[0];
  REQUIRE(m0.membraneID == "c1_c2");
  REQUIRE(m0.compA->getId() == "c1");
  REQUIRE(m0.compB->getId() == "c2");
  REQUIRE(m0.indexPair.size() == 1);
  REQUIRE(m0.indexPair[0] == std::pair<std::size_t, std::size_t>{0, 0});

  geometry::Membrane &m1 = s.membraneVec[1];
  REQUIRE(m1.membraneID == "c2_c3");
  REQUIRE(m1.compA->getId() == "c2");
  REQUIRE(m1.compB->getId() == "c3");
  REQUIRE(m1.indexPair.size() == 1);
  REQUIRE(m1.indexPair[0] == std::pair<std::size_t, std::size_t>{0, 0});

  simulate::Simulation sim(s, simulate::SimulatorType::Pixel);

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

  double dt = 0.134521234;
  double volC1 = 10.0;
  WHEN("single Euler step") {
    sim.doTimestep(dt, dt);
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
    sim.doTimestep(dt, dt);
    double A_c2 = sim.getAvgMinMax(1, 1, 0).avg;
    sim.doTimestep(dt, dt);
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
    sim.doTimestep(dt, dt);
    sim.doTimestep(dt, dt);
    double A_c2 = sim.getAvgMinMax(2, 1, 0).avg;
    double A_c3 = sim.getAvgMinMax(2, 2, 0).avg;
    sim.doTimestep(dt, dt);
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
    sim.doTimestep(1000, 0.20138571);
    std::size_t it = sim.getTimePoints().size() - 1;
    double A_c1 = 1.0;
    double A_c2 = sim.getAvgMinMax(it, 1, 0).avg;
    double A_c3 = sim.getAvgMinMax(it, 2, 0).avg;
    double B_c1 = sim.getAvgMinMax(it, 0, 0).avg;
    double B_c2 = sim.getAvgMinMax(it, 1, 1).avg;
    double B_c3 = sim.getAvgMinMax(it, 2, 1).avg;

    // check concentration values
    REQUIRE(A_c1 == Approx(1.0).epsilon(acceptable_error));
    REQUIRE(
        A_c2 ==
        Approx(0.5 * A_c1 * (0.06 + 0.10) / 0.06).epsilon(acceptable_error));
    REQUIRE(A_c3 == Approx(A_c2 - A_c1).epsilon(acceptable_error));
    // B_c1 "steady state" solution is linear growth
    REQUIRE(B_c3 ==
            Approx((0.06 / 0.10) * A_c3 / 0.2).epsilon(acceptable_error));
    REQUIRE(B_c2 == Approx(B_c3 / 2.0).epsilon(acceptable_error));

    // check concentration derivatives
    double eps = 1.e-5;
    sim.doTimestep(eps, eps);
    ++it;
    double dA2 = (sim.getAvgMinMax(it, 1, 0).avg - A_c2) / eps;
    REQUIRE(dA2 == Approx(0).epsilon(acceptable_error));
    double dA3 = (sim.getAvgMinMax(it, 2, 0).avg - A_c3) / eps;
    REQUIRE(dA3 == Approx(0).epsilon(acceptable_error));
    double dB1 = volC1 * (sim.getAvgMinMax(it, 0, 0).avg - B_c1) / eps;
    REQUIRE(dB1 == Approx(1).epsilon(acceptable_error));
    double dB2 = (sim.getAvgMinMax(it, 1, 1).avg - B_c2) / eps;
    REQUIRE(dB2 == Approx(0).epsilon(acceptable_error));
    double dB3 = (sim.getAvgMinMax(it, 2, 1).avg - B_c3) / eps;
    REQUIRE(dB3 == Approx(0).epsilon(acceptable_error));
  }
}

SCENARIO("Simulate: very_simple_model, 2d geometry",
         "[core][simulate][pixel]") {
  // import model
  sbml::SbmlDocWrapper s;
  QFile f(":/models/very-simple-model.xml");
  f.open(QIODevice::ReadOnly);
  s.importSBMLString(f.readAll().toStdString());

  // check fields have correct compartments & sizes
  geometry::Field &fa1 = s.mapSpeciesIdToField.at("A_c1");
  REQUIRE(fa1.geometry->getId() == "c1");
  REQUIRE(fa1.speciesID == "A_c1");
  geometry::Field &fb1 = s.mapSpeciesIdToField.at("B_c1");
  REQUIRE(fb1.geometry->getId() == "c1");
  REQUIRE(fb1.speciesID == "B_c1");
  geometry::Field &fa2 = s.mapSpeciesIdToField.at("A_c2");
  REQUIRE(fa2.geometry->getId() == "c2");
  REQUIRE(fa2.speciesID == "A_c2");
  geometry::Field &fb2 = s.mapSpeciesIdToField.at("B_c2");
  REQUIRE(fb2.geometry->getId() == "c2");
  REQUIRE(fb2.speciesID == "B_c2");
  geometry::Field &fa3 = s.mapSpeciesIdToField.at("A_c3");
  REQUIRE(fa3.geometry->getId() == "c3");
  REQUIRE(fa3.speciesID == "A_c3");
  geometry::Field &fb3 = s.mapSpeciesIdToField.at("B_c3");
  REQUIRE(fb3.geometry->getId() == "c3");
  REQUIRE(fb3.speciesID == "B_c3");

  // check membranes have correct compartment pairs & sizes
  geometry::Membrane &m0 = s.membraneVec[0];
  REQUIRE(m0.membraneID == "c1_c2");
  REQUIRE(m0.compA->getId() == "c1");
  REQUIRE(m0.compB->getId() == "c2");
  REQUIRE(m0.indexPair.size() == 338);

  geometry::Membrane &m1 = s.membraneVec[1];
  REQUIRE(m1.membraneID == "c2_c3");
  REQUIRE(m1.compA->getId() == "c2");
  REQUIRE(m1.compB->getId() == "c3");
  REQUIRE(m1.indexPair.size() == 108);

  simulate::Simulation sim(s, simulate::SimulatorType::Pixel);

  // check initial concentrations:
  // note: A_c1 is constant, so not part of simulation
  REQUIRE(sim.getAvgMinMax(0, 0, 0).avg == dbl_approx(0.0));
  REQUIRE(sim.getAvgMinMax(0, 1, 0).avg == dbl_approx(0.0));
  REQUIRE(sim.getAvgMinMax(0, 1, 1).avg == dbl_approx(0.0));
  REQUIRE(sim.getAvgMinMax(0, 2, 0).avg == dbl_approx(0.0));
  REQUIRE(sim.getAvgMinMax(0, 2, 1).avg == dbl_approx(0.0));

  WHEN("one Euler steps: diffusion of A into c2") {
    sim.doTimestep(0.01, 0.01);
    REQUIRE(sim.getAvgMinMax(1, 0, 0).avg == dbl_approx(0.0));
    REQUIRE(sim.getAvgMinMax(1, 1, 0).avg > 0);
    REQUIRE(sim.getAvgMinMax(1, 1, 1).avg == dbl_approx(0.0));
    REQUIRE(sim.getAvgMinMax(1, 2, 0).avg == dbl_approx(0.0));
    REQUIRE(sim.getAvgMinMax(1, 2, 1).avg == dbl_approx(0.0));
  }

  WHEN("many Euler steps: all species non-zero") {
    sim.doTimestep(1.00, 0.02);
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

SCENARIO("Simulate: single-compartment-diffusion, circular geometry",
         "[core][simulate]") {
  // see docs/tests/diffusion.rst for analytic expressions used here
  // NB central point of initial distribution: (48,99-48) <-> ix=1577

  constexpr double pi = 3.14159265358979323846;
  double sigma2 = 36.0;
  double epsilon = 1e-10;
  // import model
  sbml::SbmlDocWrapper s;
  if (QFile f(":/models/single-compartment-diffusion.xml");
      f.open(QIODevice::ReadOnly)) {
    s.importSBMLString(f.readAll().toStdString());
  }

  // check fields have correct compartments
  geometry::Field &slow = s.mapSpeciesIdToField.at("slow");
  REQUIRE(slow.geometry->getId() == "circle");
  REQUIRE(slow.speciesID == "slow");
  geometry::Field &fast = s.mapSpeciesIdToField.at("fast");
  REQUIRE(fast.geometry->getId() == "circle");
  REQUIRE(fast.speciesID == "fast");

  // check total initial concentration matches analytic value
  double analytic_total = sigma2 * pi;
  for (const auto &c : {slow.conc, fast.conc}) {
    REQUIRE(std::abs(utils::sum(c) - analytic_total) / analytic_total <
            epsilon);
  }

  // check initial distribution matches analytic one
  for (const auto &f : {slow, fast}) {
    double D = f.diffusionConstant;
    double t0 = sigma2 / 4.0 / D;
    double maxRelErr = 0;
    for (std::size_t i = 0; i < f.geometry->nPixels(); ++i) {
      const auto &p = f.geometry->getPixel(i);
      double c = analytic(p, 0, D, t0);
      double relErr = std::abs(f.conc[i] - c) / c;
      maxRelErr = std::max(maxRelErr, relErr);
    }
    CAPTURE(f.diffusionConstant);
    REQUIRE(maxRelErr < epsilon);
  }

  for (auto simType :
       {simulate::SimulatorType::Pixel, simulate::SimulatorType::DUNE}) {
    double initialRelativeError = 1e-9;
    double evolvedRelativeError = 0.01;
    double dt = 0.02;
    if (simType == simulate::SimulatorType::DUNE) {
      initialRelativeError = 0.05;
      evolvedRelativeError = 0.2;
      dt = 1.0;
    }

    // integrate & compare
    simulate::Simulation sim(s, simType);
    double t = 10.0;
    for (std::size_t step = 0; step < 2; ++step) {
      sim.doTimestep(t, dt);
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
    std::vector<double> D{slow.diffusionConstant, fast.diffusionConstant};
    std::size_t timeIndex = sim.getTimePoints().size() - 1;
    t = sim.getTimePoints().back();
    for (auto speciesIndex : {std::size_t{0}, std::size_t{1}}) {
      double t0 = sigma2 / 4.0 / D[speciesIndex];
      auto conc = sim.getConc(timeIndex, 0, speciesIndex);
      double maxRelErr = 0;
      for (std::size_t i = 0; i < slow.geometry->nPixels(); ++i) {
        const auto &p = slow.geometry->getPixel(i);
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

SCENARIO("Simulate: small-single-compartment-diffusion, circular geometry",
         "[core][simulate]") {
  WHEN("many steps: both species end up equally & uniformly distributed") {
    double epsilon = 1e-6;
    // import model
    sbml::SbmlDocWrapper s;
    if (QFile f(":/models/small-single-compartment-diffusion.xml");
        f.open(QIODevice::ReadOnly)) {
      s.importSBMLString(f.readAll().toStdString());
    }
    for (auto simulator :
         {simulate::SimulatorType::DUNE, simulate::SimulatorType::Pixel}) {
      auto sim = simulate::Simulation(s, simulator);
      double dt = 0.05;
      if (simulator == simulate::SimulatorType::DUNE) {
        dt = 0.5;
      }
      sim.doTimestep(50.0, dt);
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

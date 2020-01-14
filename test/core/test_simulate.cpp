#include <sbml/SBMLDocument.h>
#include <sbml/SBMLReader.h>
#include <sbml/SBMLWriter.h>

#include <QFile>
#include <algorithm>

#include "catch_wrapper.hpp"
#include "sbml.hpp"
#include "sbml_test_data/very_simple_model.hpp"
#include "simulate.hpp"

SCENARIO("Simulate: very_simple_model, single pixel geometry",
         "[core][simulate]") {
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

  simulate::Simulate sim(&s);
  // add fields
  for (const auto &compartmentID : s.compartments) {
    sim.addCompartment(&s.mapCompIdToGeometry.at(compartmentID));
  }
  // add membranes
  for (auto &membrane : s.membraneVec) {
    sim.addMembrane(&membrane);
  }

  // check initial concentrations:
  // note A_c1 is a constant, so it does not have a field,
  // and the first field in sim is B_c1
  REQUIRE(sim.field[0] == &fb1);
  REQUIRE(sim.field[0]->conc[0] == dbl_approx(0.0));
  REQUIRE(sim.field[0]->getMeanConcentration() == dbl_approx(0.0));
  REQUIRE(fa1.getMeanConcentration() == dbl_approx(1.0));
  REQUIRE(fb1.getMeanConcentration() == dbl_approx(0.0));
  REQUIRE(fa2.getMeanConcentration() == dbl_approx(0.0));
  REQUIRE(fb2.getMeanConcentration() == dbl_approx(0.0));
  REQUIRE(fa3.getMeanConcentration() == dbl_approx(0.0));
  REQUIRE(fb3.getMeanConcentration() == dbl_approx(0.0));

  // check initial concentration image
  img = sim.getConcentrationImage();
  REQUIRE(img.size() == QSize(1, 3));
  REQUIRE(img.pixel(0, 0) == QColor(0, 0, 0).rgba());
  REQUIRE(img.pixel(0, 1) == QColor(0, 0, 0).rgba());
  REQUIRE(img.pixel(0, 2) == QColor(0, 0, 0).rgba());

  double dt = 0.134521234;
  double volC1 = 10.0;
  WHEN("single Euler step") {
    sim.integrateForwardsEuler(dt);
    // A_c1 = 1 = const
    REQUIRE(fa1.getMeanConcentration() == 1.0);
    // B_c1 = 0
    REQUIRE(fb1.getMeanConcentration() == 0.0);
    // A_c2 += k_1 A_c1 dt
    REQUIRE(fa2.getMeanConcentration() == dbl_approx(0.1 * 1.0 * dt));
    // B_c2 = 0
    REQUIRE(fb2.getMeanConcentration() == 0.0);
    // A_c3 = 0
    REQUIRE(fa3.getMeanConcentration() == 0.0);
    // B_c3 = 0
    REQUIRE(fb3.getMeanConcentration() == 0.0);
  }

  WHEN("two Euler steps") {
    sim.integrateForwardsEuler(dt);
    double A_c2 = fa2.getMeanConcentration();
    sim.integrateForwardsEuler(dt);
    // A_c1 = 1 = const
    REQUIRE(fa1.getMeanConcentration() == 1.0);
    // B_c1 = 0
    REQUIRE(fb1.getMeanConcentration() == 0.0);
    // A_c2 += k_1 A_c1 dt - k1 * A_c2 * dt
    REQUIRE(fa2.getMeanConcentration() ==
            dbl_approx(A_c2 + 0.1 * dt - A_c2 * 0.1 * dt));
    // B_c2 = 0
    REQUIRE(fb2.getMeanConcentration() == 0.0);
    // A_c3 += k_1 A_c2 dt / c3
    REQUIRE(fa3.getMeanConcentration() == dbl_approx(0.1 * A_c2 * dt));
    // B_c3 = 0
    REQUIRE(fb3.getMeanConcentration() == 0.0);
  }

  WHEN("three Euler steps") {
    sim.integrateForwardsEuler(dt);
    sim.integrateForwardsEuler(dt);
    double A_c2 = fa2.getMeanConcentration();
    double A_c3 = fa3.getMeanConcentration();
    sim.integrateForwardsEuler(dt);
    // A_c1 = 1 = const
    REQUIRE(fa1.getMeanConcentration() == 1.0);
    // B_c1 = 0
    REQUIRE(fb1.getMeanConcentration() == 0.0);
    // A_c2 += k_1 (A_c1 - A_c2) dt + k2 * A_c3 * dt
    REQUIRE(fa2.getMeanConcentration() ==
            dbl_approx(A_c2 + 0.1 * dt - A_c2 * 0.1 * dt + A_c3 * 0.1 * dt));
    // B_c2 = 0
    REQUIRE(fb2.getMeanConcentration() == 0.0);
    // A_c3 += k_1 A_c2 dt - k_1 A_c3 dt -
    REQUIRE(fa3.getMeanConcentration() ==
            dbl_approx(A_c3 + (0.1 * (A_c2 - A_c3) * dt - 0.3 * A_c3 * dt)));
    // B_c3 = 0.2 * 0.3 * A_c3 * dt
    REQUIRE(fb3.getMeanConcentration() == dbl_approx(0.3 * A_c3 * dt));
  }

  WHEN("many Euler steps -> steady state solution") {
    // when A & B saturate in all compartments, we reach a steady state
    // by conservation: flux of B of into c1 = flux of A from c1 = 0.1
    // all other net fluxes are zero

    double acceptable_error = 1.e-8;
    for (int i = 0; i < 5000; ++i) {
      sim.integrateForwardsEuler(0.20138571);
    }
    double A_c1 = fa1.getMeanConcentration();
    double A_c2 = fa2.getMeanConcentration();
    double A_c3 = fa3.getMeanConcentration();
    double B_c1 = fb1.getMeanConcentration();
    double B_c2 = fb2.getMeanConcentration();
    double B_c3 = fb3.getMeanConcentration();

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
    sim.integrateForwardsEuler(eps);
    double dA1 = (fa1.getMeanConcentration() - A_c1) / eps;
    REQUIRE(dA1 == Approx(0).epsilon(acceptable_error));
    double dA2 = (fa2.getMeanConcentration() - A_c2) / eps;
    REQUIRE(dA2 == Approx(0).epsilon(acceptable_error));
    double dA3 = (fa3.getMeanConcentration() - A_c3) / eps;
    REQUIRE(dA3 == Approx(0).epsilon(acceptable_error));
    double dB1 = volC1 * (fb1.getMeanConcentration() - B_c1) / eps;
    REQUIRE(dB1 == Approx(1).epsilon(acceptable_error));
    double dB2 = (fb2.getMeanConcentration() - B_c2) / eps;
    REQUIRE(dB2 == Approx(0).epsilon(acceptable_error));
    double dB3 = (fb3.getMeanConcentration() - B_c3) / eps;
    REQUIRE(dB3 == Approx(0).epsilon(acceptable_error));
  }
}

SCENARIO("Simulate: very_simple_model, 2d geometry", "[core][simulate]") {
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

  simulate::Simulate sim(&s);
  // add fields
  for (const auto &compartmentID : s.compartments) {
    sim.addCompartment(&s.mapCompIdToGeometry.at(compartmentID));
  }
  // add membranes
  for (auto &membrane : s.membraneVec) {
    sim.addMembrane(&membrane);
  }

  // check initial concentrations:
  // note A_c1 is a constant, so it does not have a field,
  // and the first field in sim is B_c1
  REQUIRE(sim.field[0] == &fb1);
  REQUIRE(sim.field[0]->conc[0] == dbl_approx(0.0));
  REQUIRE(sim.field[0]->getMeanConcentration() == dbl_approx(0.0));
  REQUIRE(fa1.getMeanConcentration() == dbl_approx(1.0));
  REQUIRE(fb1.getMeanConcentration() == dbl_approx(0.0));
  REQUIRE(fa2.getMeanConcentration() == dbl_approx(0.0));
  REQUIRE(fb2.getMeanConcentration() == dbl_approx(0.0));
  REQUIRE(fa3.getMeanConcentration() == dbl_approx(0.0));
  REQUIRE(fb3.getMeanConcentration() == dbl_approx(0.0));

  WHEN("one Euler steps: diffusion of A into c2") {
    sim.integrateForwardsEuler(0.01);
    REQUIRE(fa1.getMeanConcentration() == dbl_approx(1.0));
    REQUIRE(fb1.getMeanConcentration() == dbl_approx(0.0));
    REQUIRE(fa2.getMeanConcentration() > 0);
    REQUIRE(fb2.getMeanConcentration() == dbl_approx(0.0));
    REQUIRE(fa3.getMeanConcentration() == dbl_approx(0.0));
    REQUIRE(fb3.getMeanConcentration() == dbl_approx(0.0));
  }

  WHEN("many Euler steps: all species non-zero") {
    for (int i = 0; i < 50; ++i) {
      sim.integrateForwardsEuler(0.02);
    }
    REQUIRE(fa1.getMeanConcentration() == dbl_approx(1.0));
    REQUIRE(fb1.getMeanConcentration() > 0);
    REQUIRE(fa2.getMeanConcentration() > 0);
    REQUIRE(fb2.getMeanConcentration() > 0);
    REQUIRE(fa3.getMeanConcentration() > 0);
    REQUIRE(fb3.getMeanConcentration() > 0);
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
  // initial distribution: u(r) = exp(-r^2/sigma^2)
  // where r^2 = (x-48^2) + (y-48)^2,
  // and sigma = 6

  // 2d isotropic diffusion: u(r,t) = A exp{-r^2 / (4 D t)} / (4 pi D t)
  // so matching this expression to initial condition above:
  // A = pi sigma^2  <-- total amount: conserved quantity
  // t0 = sigma^2/4D  <-- analytic time where our simulation starts
  // u(t) = [t0/(t+t0)] * exp(-r^2/4Dt)  <-- analytic prediction

  // NB central point: (48,99-48) <-> ix=1577

  constexpr double pi = 3.14159265358979323846;
  double sigma2 = 36.0;
  double epsilon = 1e-6;
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

  // check total concentration matches analytic value
  double analytic_total = sigma2 * pi;
  for (const auto &c : {slow.conc, fast.conc}) {
    double sum = std::accumulate(cbegin(c), cend(c), 0.0);
    REQUIRE(std::abs(sum - analytic_total) / analytic_total < epsilon);
  }

  // check initial distribution matches analytic one
  for (const auto &f : {slow, fast}) {
    double D = f.diffusionConstant;
    double t0 = sigma2 / 4.0 / D;
    for (std::size_t i = 0; i < f.geometry->nPixels(); ++i) {
      const auto &p = f.geometry->getPixel(i);
      double c = analytic(p, 0, D, t0);
      REQUIRE(std::abs(f.conc[i] - c) / c < epsilon);
    }
  }

  // integrate & compare
  simulate::Simulate sim(&s);
  sim.addCompartment(&s.mapCompIdToGeometry.at("circle"));
  double t = 0;
  double dt = 0.02;
  for (int step = 0; step < 2; ++step) {
    for (int k = 0; k < static_cast<int>(10.0 / dt); ++k) {
      sim.integrateForwardsEuler(dt);
      t += dt;
    }
    // check total concentration matches conserved analytic value
    for (const auto &c : {slow.conc, fast.conc}) {
      double sum = std::accumulate(cbegin(c), cend(c), 0.0);
      REQUIRE(std::abs(sum - analytic_total) / analytic_total < epsilon);
    }
    // check new distribution matches analytic one
    for (const auto &f : {slow, fast}) {
      double D = f.diffusionConstant;
      double t0 = sigma2 / 4.0 / D;
      for (std::size_t i = 0; i < f.geometry->nPixels(); ++i) {
        const auto &p = f.geometry->getPixel(i);
        // only check part within a radius of 16 units from centre to avoid
        // boundary effects: analytic solution is in infinite volume
        if (r2(p) < 16 * 16) {
          double c0 = analytic(p, 0, D, t0);
          double c = analytic(p, t, D, t0);
          double dc_analytic = c - c0;
          double dc_measured = f.conc[i] - f.init[i];
          CAPTURE(t);
          CAPTURE(r2(p));
          CAPTURE(p);
          CAPTURE(c0);
          CAPTURE(c);
          CAPTURE(f.conc[i]);
          CAPTURE(f.init[i]);
          // relative error on *change* in concentration vs analytic < 2%
          REQUIRE(std::abs(dc_measured - dc_analytic) / dc_analytic < 0.10);
        }
      }
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
    simulate::Simulate sim(&s);
    sim.addCompartment(&s.mapCompIdToGeometry.at("circle"));
    geometry::Field &slow = s.mapSpeciesIdToField.at("slow");
    geometry::Field &fast = s.mapSpeciesIdToField.at("fast");
    for (int j = 0; j < 1000; ++j) {
      sim.integrateForwardsEuler(0.05);
    }
    // after many steps in a finite volume, diffusion has reached the limiting
    // case of a uniform distribution
    for (const auto &c : {slow.conc, fast.conc}) {
      auto pair = std::minmax_element(c.cbegin(), c.cend());
      double min = *pair.first;
      double max = *pair.second;
      double av = std::accumulate(c.cbegin(), c.cend(), 0.0) /
                  static_cast<double>(c.size());
      CAPTURE(min);
      CAPTURE(max);
      CAPTURE(av);
      REQUIRE(std::abs((min - av) / av) < epsilon);
      REQUIRE(std::abs((max - av) / av) < epsilon);
    }
  }
}

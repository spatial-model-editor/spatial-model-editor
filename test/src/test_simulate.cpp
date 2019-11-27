#include <sbml/SBMLTypes.h>

#include <QFile>

#include "catch_wrapper.hpp"
#include "logger.hpp"
#include "sbml.hpp"
#include "sbml_test_data/very_simple_model.hpp"
#include "simulate.hpp"

SCENARIO("Simulate: very_simple_model, single pixel geometry",
         "[simulate][non-gui]") {
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
  REQUIRE(fa1.geometry->compartmentID == "c1");
  REQUIRE(fa1.speciesID == "A_c1");
  geometry::Field &fb1 = s.mapSpeciesIdToField.at("B_c1");
  REQUIRE(fb1.geometry->compartmentID == "c1");
  REQUIRE(fb1.speciesID == "B_c1");
  geometry::Field &fa2 = s.mapSpeciesIdToField.at("A_c2");
  REQUIRE(fa2.geometry->compartmentID == "c2");
  REQUIRE(fa2.speciesID == "A_c2");
  geometry::Field &fb2 = s.mapSpeciesIdToField.at("B_c2");
  REQUIRE(fb2.geometry->compartmentID == "c2");
  REQUIRE(fb2.speciesID == "B_c2");
  geometry::Field &fa3 = s.mapSpeciesIdToField.at("A_c3");
  REQUIRE(fa3.geometry->compartmentID == "c3");
  REQUIRE(fa3.speciesID == "A_c3");
  geometry::Field &fb3 = s.mapSpeciesIdToField.at("B_c3");
  REQUIRE(fb3.geometry->compartmentID == "c3");
  REQUIRE(fb3.speciesID == "B_c3");

  // check membranes have correct compartment pairs & sizes
  geometry::Membrane &m0 = s.membraneVec[0];
  REQUIRE(m0.membraneID == "c1_c2");
  REQUIRE(m0.compA->compartmentID == "c1");
  REQUIRE(m0.compB->compartmentID == "c2");
  REQUIRE(m0.indexPair.size() == 1);
  REQUIRE(m0.indexPair[0] == std::pair<std::size_t, std::size_t>{0, 0});

  geometry::Membrane &m1 = s.membraneVec[1];
  REQUIRE(m1.membraneID == "c2_c3");
  REQUIRE(m1.compA->compartmentID == "c2");
  REQUIRE(m1.compB->compartmentID == "c3");
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

SCENARIO("Simulate: very_simple_model, 2d geometry", "[simulate][non-gui]") {
  // import model
  sbml::SbmlDocWrapper s;
  QFile f(":/models/very-simple-model.xml");
  f.open(QIODevice::ReadOnly);
  s.importSBMLString(f.readAll().toStdString());

  // check fields have correct compartments & sizes
  geometry::Field &fa1 = s.mapSpeciesIdToField.at("A_c1");
  REQUIRE(fa1.geometry->compartmentID == "c1");
  REQUIRE(fa1.speciesID == "A_c1");
  geometry::Field &fb1 = s.mapSpeciesIdToField.at("B_c1");
  REQUIRE(fb1.geometry->compartmentID == "c1");
  REQUIRE(fb1.speciesID == "B_c1");
  geometry::Field &fa2 = s.mapSpeciesIdToField.at("A_c2");
  REQUIRE(fa2.geometry->compartmentID == "c2");
  REQUIRE(fa2.speciesID == "A_c2");
  geometry::Field &fb2 = s.mapSpeciesIdToField.at("B_c2");
  REQUIRE(fb2.geometry->compartmentID == "c2");
  REQUIRE(fb2.speciesID == "B_c2");
  geometry::Field &fa3 = s.mapSpeciesIdToField.at("A_c3");
  REQUIRE(fa3.geometry->compartmentID == "c3");
  REQUIRE(fa3.speciesID == "A_c3");
  geometry::Field &fb3 = s.mapSpeciesIdToField.at("B_c3");
  REQUIRE(fb3.geometry->compartmentID == "c3");
  REQUIRE(fb3.speciesID == "B_c3");

  // check membranes have correct compartment pairs & sizes
  geometry::Membrane &m0 = s.membraneVec[0];
  REQUIRE(m0.membraneID == "c1_c2");
  REQUIRE(m0.compA->compartmentID == "c1");
  REQUIRE(m0.compB->compartmentID == "c2");
  REQUIRE(m0.indexPair.size() == 338);

  geometry::Membrane &m1 = s.membraneVec[1];
  REQUIRE(m1.membraneID == "c2_c3");
  REQUIRE(m1.compA->compartmentID == "c2");
  REQUIRE(m1.compB->compartmentID == "c3");
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

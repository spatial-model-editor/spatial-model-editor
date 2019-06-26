#include "catch_qt.h"
#include "sbml_test_data/very_simple_model.h"

#include "sbml.h"
#include "simulate.h"

SCENARIO("simulate very_simple_model.xml", "[simulate][non-gui]") {
  // import model
  std::unique_ptr<libsbml::SBMLDocument> doc(
      libsbml::readSBMLFromString(sbml_test_data::very_simple_model().xml));
  libsbml::SBMLWriter().writeSBML(doc.get(), "tmp.xml");
  SbmlDocWrapper s;
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
  s.setCompartmentColour("c1", col1);
  s.setCompartmentColour("c2", col2);
  s.setCompartmentColour("c3", col3);

  // check we have identified the compartments and membranes
  REQUIRE(s.compartments == QStringList{"c1", "c2", "c3"});
  REQUIRE(s.membranes == QStringList{"c1-c2", "c2-c3"});

  // check fields have correct compartments & sizes
  geometry::Field &f1 = s.mapCompIdToField.at("c1");
  REQUIRE(f1.geometry->compartmentID == "c1");
  REQUIRE(f1.n_pixels == 1);
  REQUIRE(f1.n_species == 2);
  geometry::Field &f2 = s.mapCompIdToField.at("c2");
  REQUIRE(f2.geometry->compartmentID == "c2");
  REQUIRE(f2.n_pixels == 1);
  REQUIRE(f2.n_species == 2);
  geometry::Field &f3 = s.mapCompIdToField.at("c3");
  REQUIRE(f3.geometry->compartmentID == "c3");
  REQUIRE(f3.n_pixels == 1);
  REQUIRE(f3.n_species == 2);
  simulate::Simulate sim(&s);

  // check membranes have correct compartment pairs & sizes
  geometry::Membrane &m0 = s.membraneVec[0];
  REQUIRE(m0.membraneID == "c1-c2");
  REQUIRE(m0.fieldA->geometry->compartmentID == "c1");
  REQUIRE(m0.fieldB->geometry->compartmentID == "c2");
  REQUIRE(m0.indexPair.size() == 1);
  REQUIRE(m0.indexPair[0] == std::pair<std::size_t, std::size_t>{0, 0});

  geometry::Membrane &m1 = s.membraneVec[1];
  REQUIRE(m1.membraneID == "c2-c3");
  REQUIRE(m1.fieldA->geometry->compartmentID == "c2");
  REQUIRE(m1.fieldB->geometry->compartmentID == "c3");
  REQUIRE(m1.indexPair.size() == 1);
  REQUIRE(m1.indexPair[0] == std::pair<std::size_t, std::size_t>{0, 0});

  // add fields
  for (const auto &compartmentID : s.compartments) {
    sim.addField(&s.mapCompIdToField.at(compartmentID));
  }
  // add membranes
  for (auto &membrane : s.membraneVec) {
    sim.addMembrane(&membrane);
  }

  // check initial concentrations: A_c1=1=fixed, the rest zero
  REQUIRE(f1.getMeanConcentration(0) == 1.0);
  REQUIRE(f1.getMeanConcentration(1) == 0.0);
  REQUIRE(f2.getMeanConcentration(0) == 0.0);
  REQUIRE(f2.getMeanConcentration(1) == 0.0);
  REQUIRE(f3.getMeanConcentration(0) == 0.0);
  REQUIRE(f3.getMeanConcentration(1) == 0.0);

  // check initial conentration image
  img = sim.getConcentrationImage();
  REQUIRE(img.size() == QSize(1, 3));
  REQUIRE(img.pixel(0, 0) == sim.speciesColour[0].rgba());
  REQUIRE(img.pixel(0, 1) == QColor(0, 0, 0).rgba());
  REQUIRE(img.pixel(0, 2) == QColor(0, 0, 0).rgba());

  double dt = 0.134521234;
  WHEN("single Euler step") {
    sim.integrateForwardsEuler(dt);
    // A_c1 = 1 = const
    REQUIRE(f1.getMeanConcentration(0) == 1.0);
    // B_c1 = 0
    REQUIRE(f1.getMeanConcentration(1) == 0.0);
    // A_c2 += k_1 A_c1 dt
    REQUIRE(f2.getMeanConcentration(0) == dbl_approx(0.1 * 1.0 * dt));
    // B_c2 = 0
    REQUIRE(f2.getMeanConcentration(1) == 0.0);
    // A_c3 = 0
    REQUIRE(f3.getMeanConcentration(0) == 0.0);
    // B_c3 = 0
    REQUIRE(f3.getMeanConcentration(1) == 0.0);
  }

  WHEN("two Euler steps") {
    sim.integrateForwardsEuler(dt);
    double A_c2 = f2.getMeanConcentration(0);
    sim.integrateForwardsEuler(dt);
    // A_c1 = 1 = const
    REQUIRE(f1.getMeanConcentration(0) == 1.0);
    // B_c1 = 0
    REQUIRE(f1.getMeanConcentration(1) == 0.0);
    // A_c2 += k_1 A_c1 dt - k1 * A_c2 * dt
    REQUIRE(f2.getMeanConcentration(0) ==
            dbl_approx(A_c2 + 0.1 * dt - A_c2 * 0.1 * dt));
    // B_c2 = 0
    REQUIRE(f2.getMeanConcentration(1) == 0.0);
    // A_c3 += k_1 A_c2 dt
    REQUIRE(f3.getMeanConcentration(0) == dbl_approx(0.1 * A_c2 * dt));
    // B_c3 = 0
    REQUIRE(f3.getMeanConcentration(1) == 0.0);
  }

  WHEN("three Euler steps") {
    sim.integrateForwardsEuler(dt);
    sim.integrateForwardsEuler(dt);
    double A_c2 = f2.getMeanConcentration(0);
    double A_c3 = f3.getMeanConcentration(0);
    sim.integrateForwardsEuler(dt);
    // A_c1 = 1 = const
    REQUIRE(f1.getMeanConcentration(0) == 1.0);
    // B_c1 = 0
    REQUIRE(f1.getMeanConcentration(1) == 0.0);
    // A_c2 += k_1 (A_c1 - A_c2) dt + k2 * A_c3 * dt
    REQUIRE(f2.getMeanConcentration(0) ==
            dbl_approx(A_c2 + 0.1 * dt - A_c2 * 0.1 * dt + A_c3 * 0.1 * dt));
    // B_c2 = 0
    REQUIRE(f2.getMeanConcentration(1) == 0.0);
    // A_c3 += k_1 A_c2 dt - k_1 A_c3 dt -
    REQUIRE(
        f3.getMeanConcentration(0) ==
        dbl_approx(A_c3 + 0.1 * (A_c2 - A_c3) * dt - 0.2 * 0.3 * A_c3 * dt));
    // B_c3 = 0.2 * 0.3 * A_c3 * dt
    REQUIRE(f3.getMeanConcentration(1) == dbl_approx(0.2 * 0.3 * A_c3 * dt));
  }

  WHEN("many Euler steps -> steady state solution") {
    // when A & B saturate in all compartments, we reach a steady state
    // by conservation: flux of B of into c1 = flux of A from c1 = 0.1
    // all other net fluxes are zero

    double acceptable_error = 1.e-8;
    for (int i = 0; i < 5000; ++i) {
      sim.integrateForwardsEuler(0.2);
    }
    double A_c1 = f1.getMeanConcentration(0);
    double A_c2 = f2.getMeanConcentration(0);
    double A_c3 = f3.getMeanConcentration(0);
    double B_c1 = f1.getMeanConcentration(1);
    double B_c2 = f2.getMeanConcentration(1);
    double B_c3 = f3.getMeanConcentration(1);

    // check concentration values
    REQUIRE(A_c1 == Approx(1.0).epsilon(acceptable_error));
    REQUIRE(A_c2 ==
            Approx(A_c1 * (0.06 + 0.10) / 0.06).epsilon(acceptable_error));
    REQUIRE(A_c3 == Approx(A_c2 - A_c1).epsilon(acceptable_error));
    // B_c1 "steady state" solution is linear growth
    REQUIRE(B_c3 == Approx((0.06 / 0.10) * A_c3).epsilon(acceptable_error));
    REQUIRE(B_c2 == Approx(B_c3 / 2.0).epsilon(acceptable_error));

    // check concentration derivatives
    double eps = 1.e-5;
    sim.integrateForwardsEuler(eps);
    double dA1 = (f1.getMeanConcentration(0) - A_c1) / eps;
    REQUIRE(dA1 == Approx(0).epsilon(acceptable_error));
    double dA2 = (f2.getMeanConcentration(0) - A_c2) / eps;
    REQUIRE(dA2 == Approx(0).epsilon(acceptable_error));
    double dA3 = (f3.getMeanConcentration(0) - A_c3) / eps;
    REQUIRE(dA3 == Approx(0).epsilon(acceptable_error));
    double dB1 = (f1.getMeanConcentration(1) - B_c1) / eps;
    REQUIRE(dB1 == Approx(0.1).epsilon(acceptable_error));
    double dB2 = (f2.getMeanConcentration(1) - B_c2) / eps;
    REQUIRE(dB2 == Approx(0).epsilon(acceptable_error));
    double dB3 = (f3.getMeanConcentration(1) - B_c3) / eps;
    REQUIRE(dB3 == Approx(0).epsilon(acceptable_error));
  }
}

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

  // add fields
  for (const auto &compartmentID : s.compartments) {
    sim.addField(&s.mapCompIdToField.at(compartmentID));
  }
  // add membranes
  for (auto &membrane : s.membraneVec) {
    sim.addMembrane(&membrane);
  }
  // do a single Euler step
  sim.integrateForwardsEuler(0.001);

  REQUIRE(1 == 1);
}

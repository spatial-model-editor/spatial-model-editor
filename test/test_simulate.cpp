#include "catch.hpp"
#include "catch_qt_ostream.hpp"

#include "sbml.h"
#include "sbml_test_data/very_simple_model.h"
#include "simulate.h"

TEST_CASE("simulate very_simple_model.xml", "[simulate][non-gui]") {
  // import model
  std::unique_ptr<libsbml::SBMLDocument> doc(libsbml::readSBMLFromString(
      sbml_test_data::very_simple_model().xml().c_str()));
  // write SBML document to file
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
  s.setCompartmentColour("c2", col1);
  s.setCompartmentColour("c3", col1);

  simulate::Simulate sim(&s);
  // add fields
  for (const auto &compartmentID : s.compartments) {
    sim.addField(&s.mapCompIdToField.at(compartmentID));
  }
  // add membranes
  for (auto &membrane : s.membraneVec) {
    sim.addMembrane(&membrane);
  }

  REQUIRE(1 == 1);
}

#include "catch_qt.h"

#include "geometry.h"

TEST_CASE("one pixel compartment, one species field", "[geometry][non-gui]") {
  QImage img(1, 1, QImage::Format_RGB32);
  QRgb col = QColor(12, 243, 154).rgba();
  img.setPixel(0, 0, col);
  geometry::Compartment comp("comp", img, col);
  REQUIRE(comp.getCompartmentImage().size() == img.size());
  REQUIRE(comp.ix.size() == 1);
  REQUIRE(comp.ix[0] == QPoint(0, 0));

  geometry::Field field;
  field.init(&comp, std::vector<std::string>{"spec1"});
  field.setConstantConcentration(0, 1.3);
  REQUIRE(field.n_pixels == 1);
  REQUIRE(field.n_species == 1);
  REQUIRE(field.conc[0] == Approx(1.3));
  // Diff op on single site is a no-op: dcdt = 0
  field.applyDiffusionOperator();
  REQUIRE(field.dcdt[0] == Approx(0));
}

TEST_CASE("two pixel compartment, three species field", "[geometry][non-gui]") {
  QImage img(6, 7, QImage::Format_RGB32);
  QRgb colBG = QColor(112, 43, 4).rgba();
  QRgb col = QColor(12, 12, 12).rgba();
  img.fill(colBG);
  img.setPixel(3, 3, col);
  img.setPixel(3, 4, col);
  geometry::Compartment comp("comp", img, col);
  REQUIRE(comp.getCompartmentImage().size() == img.size());
  REQUIRE(comp.ix.size() == 2);
  REQUIRE(comp.ix[0] == QPoint(3, 3));
  REQUIRE(comp.ix[1] == QPoint(3, 4));

  geometry::Field field;
  field.init(&comp, std::vector<std::string>{"s1", "s2", "s3"});
  field.setConstantConcentration(0, 1.3);
  field.setConstantConcentration(1, 0.3);
  field.setConstantConcentration(2, 2.0);
  REQUIRE(field.n_pixels == 2);
  REQUIRE(field.n_species == 3);
  // (3,3)
  REQUIRE(field.conc[0] == Approx(1.3));
  REQUIRE(field.conc[1] == Approx(0.3));
  REQUIRE(field.conc[2] == Approx(2.0));
  // (3,4)
  REQUIRE(field.conc[3] == Approx(1.3));
  REQUIRE(field.conc[4] == Approx(0.3));
  REQUIRE(field.conc[5] == Approx(2.0));
  // Diff op on uniform distribution is also a no-op:
  field.applyDiffusionOperator();
  REQUIRE(field.dcdt[0] == Approx(0));
  REQUIRE(field.dcdt[1] == Approx(0));
  REQUIRE(field.dcdt[2] == Approx(0));
  REQUIRE(field.dcdt[3] == Approx(0));
  REQUIRE(field.dcdt[4] == Approx(0));
  REQUIRE(field.dcdt[5] == Approx(0));
}

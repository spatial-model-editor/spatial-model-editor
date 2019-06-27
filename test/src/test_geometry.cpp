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

  geometry::Field field(&comp, "spec1");
  field.setConstantConcentration(1.3);
  REQUIRE(field.conc.size() == 1);
  REQUIRE(field.conc[0] == dbl_approx(1.3));
  REQUIRE(field.dcdt.size() == 1);
  REQUIRE(field.dcdt[0] == dbl_approx(0));
  // Diff op on single site is a no-op: dcdt = 0
  field.applyDiffusionOperator();
  REQUIRE(field.dcdt[0] == dbl_approx(0));
}

TEST_CASE("two pixel compartment, one species field", "[geometry][non-gui]") {
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

  geometry::Field field(&comp, "s1");
  field.setConstantConcentration(1.3);
  REQUIRE(field.conc.size() == 2);
  // (3,3)
  REQUIRE(field.conc[0] == dbl_approx(1.3));
  // (3,4)
  REQUIRE(field.conc[1] == dbl_approx(1.3));
  // Diff op on uniform distribution is also a no-op:
  field.applyDiffusionOperator();
  REQUIRE(field.dcdt[0] == dbl_approx(0));
  REQUIRE(field.dcdt[1] == dbl_approx(0));
}

#include "catch.hpp"
#include "catch_qt_ostream.hpp"

#include "geometry.h"

TEST_CASE("single pixel compartment", "[geometry]") {
  QImage img(1, 1, QImage::Format_RGB32);
  QRgb col = QColor(12, 243, 154).rgba();
  img.setPixel(0, 0, col);
  geometry::Compartment comp("comp", img, col);
  REQUIRE(comp.getCompartmentImage().size() == QSize(1, 1));
  REQUIRE(comp.ix.size() == 1);
  REQUIRE(comp.ix[0] == QPoint(0, 0));
}

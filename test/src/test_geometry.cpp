#include "catch_wrapper.hpp"
#include "geometry.hpp"
#include "logger.hpp"

TEST_CASE("one pixel compartment, 1x1 image", "[geometry][non-gui]") {
  QImage img(1, 1, QImage::Format_RGB32);
  QRgb col = QColor(12, 243, 154).rgba();
  img.setPixel(0, 0, col);
  geometry::Compartment comp("comp", img, col);
  REQUIRE(comp.getCompartmentImage().size() == img.size());
  REQUIRE(comp.ix.size() == 1);
  REQUIRE(comp.ix[0] == QPoint(0, 0));

  QColor specCol = QColor(123, 12, 1);
  geometry::Field field(&comp, "spec1", 1.0, specCol);
  field.setUniformConcentration(1.3);
  REQUIRE(field.conc.size() == 1);
  REQUIRE(field.conc[0] == dbl_approx(1.3));
  REQUIRE(field.dcdt.size() == 1);
  REQUIRE(field.dcdt[0] == dbl_approx(0));
  QImage imgConc = field.getConcentrationImage();
  REQUIRE(imgConc.size() == img.size());
  REQUIRE(imgConc.pixelColor(0, 0) == specCol);
  // Diff op on single site is a no-op: dcdt = 0
  field.applyDiffusionOperator();
  REQUIRE(field.dcdt[0] == dbl_approx(0));

  field.importConcentration({1.23});
  REQUIRE(field.getMeanConcentration() == dbl_approx(1.23));

  auto a = field.getConcentrationArray();
  REQUIRE(a.size() == 1);
  REQUIRE(a[0] == dbl_approx(1.23));

  // set zero concentration, get image
  // (must avoid dividing by zero in normalisation)
  field.setUniformConcentration(0.0);
  REQUIRE(field.getMeanConcentration() == dbl_approx(0.0));
  REQUIRE(field.getConcentrationImage().pixelColor(0, 0) ==
          QColor(0, 0, 0, 255));
  a = field.getConcentrationArray();
  REQUIRE(a.size() == 1);
  REQUIRE(a[0] == dbl_approx(0));

  // conc array size must match image size
  REQUIRE_THROWS(field.importConcentration({1.0, 2.0}));
  REQUIRE_THROWS(field.importConcentration({}));
}

TEST_CASE("two pixel compartment, 6x7 image", "[geometry][non-gui]") {
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
  field.setUniformConcentration(1.3);
  REQUIRE(field.getMeanConcentration() == dbl_approx(1.3));
  REQUIRE(field.conc.size() == 2);
  // (3,3)
  REQUIRE(field.conc[0] == dbl_approx(1.3));
  // (3,4)
  REQUIRE(field.conc[1] == dbl_approx(1.3));

  // Diff op on uniform distribution is also a no-op:
  field.applyDiffusionOperator();
  REQUIRE(field.dcdt[0] == dbl_approx(0));
  REQUIRE(field.dcdt[1] == dbl_approx(0));

  auto a = field.getConcentrationArray();
  REQUIRE(a.size() == static_cast<std::size_t>(img.width() * img.height()));
  REQUIRE(a.front() == dbl_approx(0));
  REQUIRE(a[20] == dbl_approx(0));
  REQUIRE(a[21] == dbl_approx(1.3));
  REQUIRE(a[15] == dbl_approx(1.3));
  REQUIRE(a[23] == dbl_approx(0));
  REQUIRE(a.back() == dbl_approx(0));

  std::vector<double> arr(6 * 7, 0.0);
  arr[21] = 3.1;
  arr[15] = 9.9;
  field.importConcentration(arr);
  REQUIRE(field.getMeanConcentration() == dbl_approx((9.9 + 3.1) / 2.0));
  REQUIRE(field.conc[0] == dbl_approx(3.1));
  REQUIRE(field.conc[1] == dbl_approx(9.9));
  a = field.getConcentrationArray();
  for (std::size_t i = 0; i < a.size(); ++i) {
    REQUIRE(a[i] == dbl_approx(arr[i]));
  }

  // conc array size must match image size
  REQUIRE_THROWS(field.importConcentration({1.0, 2.0}));
  REQUIRE_THROWS(field.importConcentration({}));
}

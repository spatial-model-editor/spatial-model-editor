#include "catch_wrapper.hpp"
#include "geometry.hpp"

SCENARIO("Geometry: Compartments and Fields",
         "[core/model/geometry][core/model][core][model][geometry]") {
  GIVEN("one pixel compartment, 1x1 image") {
    QImage img(1, 1, QImage::Format_RGB32);
    auto col = qRgb(12, 243, 154);
    img.setPixel(0, 0, col);
    geometry::Compartment comp("comp", img, col);
    REQUIRE(comp.getCompartmentImage().size() == img.size());
    REQUIRE(comp.nPixels() == 1);
    REQUIRE(comp.getPixel(0) == QPoint(0, 0));

    auto specCol = qRgb(123, 12, 1);
    geometry::Field field(&comp, "spec1", 1.0, specCol);
    field.setUniformConcentration(1.3);
    REQUIRE(field.getConcentration().size() == 1);
    REQUIRE(field.getConcentration()[0] == dbl_approx(1.3));
    QImage imgConc = field.getConcentrationImage();
    REQUIRE(imgConc.size() == img.size());
    REQUIRE(imgConc.pixelColor(0, 0) == specCol);

    field.importConcentration({1.23});

    auto a = field.getConcentrationImageArray();
    REQUIRE(a.size() == 1);
    REQUIRE(a[0] == dbl_approx(1.23));

    // set zero concentration, get image
    // (must avoid dividing by zero in normalisation)
    field.setUniformConcentration(0.0);
    REQUIRE(field.getConcentrationImage().pixel(0, 0) == qRgba(0, 0, 0, 255));
    a = field.getConcentrationImageArray();
    REQUIRE(a.size() == 1);
    REQUIRE(a[0] == dbl_approx(0));

    // conc array size must match image size
    REQUIRE_THROWS(field.importConcentration({1.0, 2.0}));
    REQUIRE_THROWS(field.importConcentration({}));
  }
  GIVEN("two pixel compartment, 6x7 image") {
    QImage img(6, 7, QImage::Format_RGB32);
    auto colBG = qRgb(112, 43, 4);
    auto col = qRgb(12, 12, 12);
    img.fill(colBG);
    img.setPixel(3, 3, col);
    img.setPixel(3, 4, col);
    geometry::Compartment comp("comp", img, col);
    REQUIRE(comp.getCompartmentImage().size() == img.size());
    REQUIRE(comp.nPixels() == 2);
    REQUIRE(comp.getPixel(0) == QPoint(3, 3));
    REQUIRE(comp.getPixel(1) == QPoint(3, 4));

    geometry::Field field(&comp, "s1");
    field.setUniformConcentration(1.3);
    REQUIRE(field.getConcentration().size() == 2);
    // (3,3)
    REQUIRE(field.getConcentration()[0] == dbl_approx(1.3));
    // (3,4)
    REQUIRE(field.getConcentration()[1] == dbl_approx(1.3));

    auto a = field.getConcentrationImageArray();
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
    REQUIRE(field.getConcentration()[0] == dbl_approx(3.1));
    REQUIRE(field.getConcentration()[1] == dbl_approx(9.9));
    a = field.getConcentrationImageArray();
    for (std::size_t i = 0; i < a.size(); ++i) {
      REQUIRE(a[i] == dbl_approx(arr[i]));
    }

    // conc array size must match image size
    REQUIRE_THROWS(field.importConcentration({1.0, 2.0}));
    REQUIRE_THROWS(field.importConcentration({}));
  }
  WHEN("compartment of field is changed") {
    QImage img(6, 7, QImage::Format_RGB32);
    auto colBG = qRgb(112, 43, 4);
    auto col = qRgb(12, 12, 12);
    img.fill(colBG);
    img.setPixel(3, 3, col);
    img.setPixel(3, 4, col);
    geometry::Compartment comp1("comp1", img, colBG);
    geometry::Compartment comp2("comp2", img, col);
    REQUIRE(comp1.nPixels() == 40);
    REQUIRE(comp2.nPixels() == 2);

    geometry::Field field(&comp1, "field");
    REQUIRE(field.getCompartment() == &comp1);
    field.setUniformConcentration(2.0);
    REQUIRE(field.getConcentration().size() == comp1.nPixels());

    // changing compartment to the same one is a no-op
    field.setCompartment(&comp1);

    // changing compartment: concentration reset to zero
    field.setCompartment(&comp2);
    REQUIRE(field.getCompartment() == &comp2);
    REQUIRE(field.getConcentration().size() == comp2.nPixels());
    REQUIRE(field.getConcentration()[0] == dbl_approx(0.0));
    REQUIRE(field.getConcentration()[1] == dbl_approx(0.0));
  }
}

#include "catch_wrapper.hpp"
#include "geometry.hpp"

using namespace sme;

static void testFillMissingByDilation(std::vector<double> &arr, int w, int h) {
  const int maxIter{w + h};
  std::vector<double> arr2{arr};
  for (int iter = 0; iter < maxIter; ++iter) {
    bool finished{true};
    for (int y = 0; y < h; ++y) {
      for (int x = 0; x < w; ++x) {
        auto i{static_cast<std::size_t>(x + w * y)};
        if (arr[i] < 0) {
          // replace negative pixel with any non-negative 4-connected neighbour
          if (x > 0 && arr[i - 1] >= 0) {
            arr2[i] = arr[i - 1];
          } else if (x + 1 < w && arr[i + 1] >= 0) {
            arr2[i] = arr[i + 1];
          } else if (y > 0 && arr[i - static_cast<std::size_t>(w)] >= 0) {
            arr2[i] = arr[i - static_cast<std::size_t>(w)];
          } else if (y + 1 < h && arr[i + static_cast<std::size_t>(w)] >= 0) {
            arr2[i] = arr[i + static_cast<std::size_t>(w)];
          } else {
            // pixel has no non-negative neighbour: need another iteration
            finished = false;
          }
        }
      }
    }
    arr = arr2;
    if (finished) {
      return;
    }
  }
}

static std::vector<double> testGetConcentrationImageArray(const geometry::Field& f) {
  const auto &img = f.getCompartment()->getCompartmentImage();
  int size = img.width() * img.height();
  // NOTE: order of concentration array is [ (x=0,y=0), (x=1,y=0), ... ]
  // NOTE: (0,0) point is at bottom-left
  // NOTE: QImage has (0,0) point at top-left, so flip y-coord here
  constexpr double invalidPixelValue{-1.0};
  std::vector<double> arr(static_cast<std::size_t>(size), invalidPixelValue);
  for (std::size_t i = 0; i < f.getCompartment()->nPixels(); ++i) {
    const auto &point = f.getCompartment()->getPixel(i);
    int arrayIndex = point.x() + img.width() * (img.height() - 1 - point.y());
    arr[static_cast<std::size_t>(arrayIndex)] = f.getConcentration()[i];
  }
  testFillMissingByDilation(arr, img.width(), img.height());
  return arr;
}

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
    REQUIRE(a.front() == dbl_approx(1.3));
    REQUIRE(a[20] == dbl_approx(1.3));
    REQUIRE(a[21] == dbl_approx(1.3));
    REQUIRE(a[15] == dbl_approx(1.3));
    REQUIRE(a[23] == dbl_approx(1.3));
    REQUIRE(a.back() == dbl_approx(1.3));

    std::vector<double> arr(6 * 7, 0.0);
    arr[21] = 3.1;
    arr[15] = 9.9;
    field.importConcentration(arr);
    REQUIRE(field.getConcentration()[0] == dbl_approx(3.1));
    REQUIRE(field.getConcentration()[1] == dbl_approx(9.9));
    a = field.getConcentrationImageArray();
    REQUIRE(a[21] == dbl_approx(3.1));
    REQUIRE(a[20] == dbl_approx(3.1));
    REQUIRE(a[15] == dbl_approx(9.9));
    REQUIRE(a[14] == dbl_approx(9.9));

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
  WHEN("getConcentrationImageArray") {
    QImage img(":/geometry/concave-cell-nucleus-100x100.png");
    QRgb col0 = img.pixel(0, 0);
    QRgb col1 = img.pixel(35, 20);
    QRgb col2 = img.pixel(40, 50);
    geometry::Compartment comp0("comp0", img, col0);
    geometry::Compartment comp1("comp1", img, col1);
    geometry::Compartment comp2("comp2", img, col2);
    geometry::Field field0(&comp0, "field0");
    geometry::Field field1(&comp1, "field1");
    geometry::Field field2(&comp2, "field2");
    for (std::size_t i = 0; i < field0.getCompartment()->nPixels(); ++i) {
      field0.setConcentration(i, static_cast<double>(i) * 0.34);
    }
    for (std::size_t i = 0; i < field1.getCompartment()->nPixels(); ++i) {
      field1.setConcentration(i, static_cast<double>(i) * 0.66);
    }
    for (std::size_t i = 0; i < field2.getCompartment()->nPixels(); ++i) {
      field2.setConcentration(i, static_cast<double>(i) * 0.31 + 2.2);
    }
    auto a0{field0.getConcentrationImageArray()};
    auto t0{testGetConcentrationImageArray(field0)};
    REQUIRE(a0.size() == t0.size());
    for(std::size_t i=0; i<a0.size(); ++i){
      REQUIRE(a0[i] == dbl_approx(t0[i]));
    }
    auto a1{field1.getConcentrationImageArray()};
    auto t1{testGetConcentrationImageArray(field1)};
    REQUIRE(a1.size() == t1.size());
    for(std::size_t i=0; i<a1.size(); ++i){
      REQUIRE(a1[i] == dbl_approx(t1[i]));
    }
    auto a2{field2.getConcentrationImageArray()};
    auto t2{testGetConcentrationImageArray(field2)};
    REQUIRE(a2.size() == t2.size());
    for(std::size_t i=0; i<a2.size(); ++i){
      REQUIRE(a2[i] == dbl_approx(t2[i]));
    }
  }
}

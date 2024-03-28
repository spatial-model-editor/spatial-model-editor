#include "catch_wrapper.hpp"
#include "sme/geometry.hpp"

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

static std::vector<double>
testGetConcentrationImageArray(const geometry::Field &f) {
  const auto &img = f.getCompartment()->getCompartmentImages()[0];
  int size = img.width() * img.height();
  // NOTE: order of concentration array is [ (x=0,y=0), (x=1,y=0), ... ]
  // NOTE: (0,0) point is at bottom-left
  // NOTE: QImage has (0,0) point at top-left, so flip y-coord here
  constexpr double invalidPixelValue{-1.0};
  std::vector<double> arr(static_cast<std::size_t>(size), invalidPixelValue);
  for (std::size_t i = 0; i < f.getCompartment()->nVoxels(); ++i) {
    const auto &voxel{f.getCompartment()->getVoxel(i)};
    int arrayIndex =
        voxel.p.x() + img.width() * (img.height() - 1 - voxel.p.y());
    arr[static_cast<std::size_t>(arrayIndex)] = f.getConcentration()[i];
  }
  testFillMissingByDilation(arr, img.width(), img.height());
  return arr;
}

TEST_CASE("Geometry: Compartments and Fields",
          "[core/model/geometry][core/model][core][model][geometry]") {
  SECTION("one pixel compartment, 1x1 image") {
    common::ImageStack img{{QImage(1, 1, QImage::Format_RGB32)}};
    auto col = qRgb(12, 243, 154);
    img[0].setPixel(0, 0, col);
    geometry::Compartment comp("comp", {{img}}, col);
    REQUIRE(comp.getCompartmentImages().volume().width() == 1);
    REQUIRE(comp.getCompartmentImages().volume().height() == 1);
    REQUIRE(comp.getCompartmentImages().volume().depth() == 1);
    REQUIRE(comp.nVoxels() == 1);
    REQUIRE(comp.getVoxel(0).p == QPoint{0, 0});
    REQUIRE(comp.getVoxel(0).z == 0);

    auto specCol = qRgb(123, 12, 1);
    geometry::Field field(&comp, "spec1", 1.0, specCol);
    field.setUniformConcentration(1.3);
    REQUIRE(field.getConcentration().size() == 1);
    REQUIRE(field.getConcentration()[0] == dbl_approx(1.3));
    const auto imgConcs{field.getConcentrationImages()};
    REQUIRE(imgConcs.volume() == img.volume());
    REQUIRE(imgConcs[0].pixelColor(0, 0) == specCol);

    field.importConcentration({1.23});

    auto a = field.getConcentrationImageArray();
    REQUIRE(a.size() == 1);
    REQUIRE(a[0] == dbl_approx(1.23));

    auto a2{field.getConcentrationImageArray(true)};
    REQUIRE(a2.size() == 1);
    REQUIRE(a2[0] == dbl_approx(1.23));

    // set zero concentration, get image
    // (must avoid dividing by zero in normalisation)
    field.setUniformConcentration(0.0);
    REQUIRE(field.getConcentrationImages()[0].pixel(0, 0) ==
            qRgba(0, 0, 0, 255));
    a = field.getConcentrationImageArray();
    REQUIRE(a.size() == 1);
    REQUIRE(a[0] == dbl_approx(0));

    // conc array size must match image volume
    REQUIRE_THROWS(field.importConcentration({1.0, 2.0}));
    REQUIRE_THROWS(field.importConcentration({}));
  }
  SECTION("two pixel compartment, 6x7 image") {
    common::ImageStack img{{QImage(6, 7, QImage::Format_RGB32)}};
    auto colBG = qRgb(112, 43, 4);
    auto col = qRgb(12, 12, 12);
    img.fill(colBG);
    img[0].setPixel(3, 3, col);
    img[0].setPixel(3, 4, col);
    geometry::Compartment comp("comp", {img}, col);
    REQUIRE(comp.getCompartmentImages().volume() == img.volume());
    REQUIRE(comp.nVoxels() == 2);
    REQUIRE(comp.getVoxel(0).p == QPoint(3, 3));
    REQUIRE(comp.getVoxel(1).p == QPoint(3, 4));

    geometry::Field field(&comp, "s1");
    field.setUniformConcentration(1.3);
    REQUIRE(field.getConcentration().size() == 2);
    // (3,3)
    REQUIRE(field.getConcentration()[0] == dbl_approx(1.3));
    // (3,4)
    REQUIRE(field.getConcentration()[1] == dbl_approx(1.3));

    auto a{field.getConcentrationImageArray()};
    REQUIRE(a.size() == img.volume().nVoxels());
    REQUIRE(a.front() == dbl_approx(1.3));
    REQUIRE(a[20] == dbl_approx(1.3));
    REQUIRE(a[21] == dbl_approx(1.3)); // (3,3)
    REQUIRE(a[15] == dbl_approx(1.3)); // (3,4)
    REQUIRE(a[27] == dbl_approx(1.3));
    REQUIRE(a.back() == dbl_approx(1.3));

    // true -> inverted y-axis & zero outside of compartment
    auto a2{field.getConcentrationImageArray(true)};
    REQUIRE(a2.size() == img.volume().nVoxels());
    REQUIRE(a2.front() == dbl_approx(0.0));
    REQUIRE(a2[20] == dbl_approx(0.0));
    REQUIRE(a2[21] == dbl_approx(1.3)); // (3,3)
    REQUIRE(a2[15] == dbl_approx(0.0));
    REQUIRE(a2[27] == dbl_approx(1.3)); // (3,4)
    REQUIRE(a2.back() == dbl_approx(0));

    std::vector<double> arr(6 * 7, 0.0);
    arr[21] = 3.1; // (3,3)
    arr[15] = 9.9; // (3,4)
    field.importConcentration(arr);
    REQUIRE(field.getConcentration()[0] == dbl_approx(3.1));
    REQUIRE(field.getConcentration()[1] == dbl_approx(9.9));
    a = field.getConcentrationImageArray();
    REQUIRE(a[21] == dbl_approx(3.1));
    REQUIRE(a[20] == dbl_approx(3.1));
    REQUIRE(a[15] == dbl_approx(9.9));
    REQUIRE(a[14] == dbl_approx(9.9));

    // true -> inverted y-axis & zero outside of compartment
    a2 = field.getConcentrationImageArray(true);
    REQUIRE(a2.size() == img.volume().nVoxels());
    REQUIRE(a2.front() == dbl_approx(0.0));
    REQUIRE(a2[20] == dbl_approx(0.0));
    REQUIRE(a2[21] == dbl_approx(3.1)); // (3,3)
    REQUIRE(a2[27] == dbl_approx(9.9)); // (3,4)
    REQUIRE(a2[26] == dbl_approx(0.0));
    REQUIRE(a2.back() == dbl_approx(0.0));

    // conc array size must match image volume
    REQUIRE_THROWS(field.importConcentration({1.0, 2.0}));
    REQUIRE_THROWS(field.importConcentration({}));
  }
  SECTION("compartment of field is changed") {
    common::ImageStack img{{QImage(6, 7, QImage::Format_RGB32)}};
    auto colBG = qRgb(112, 43, 4);
    auto col = qRgb(12, 12, 12);
    img.fill(colBG);
    img[0].setPixel(3, 3, col);
    img[0].setPixel(3, 4, col);
    geometry::Compartment comp1("comp1", {img}, colBG);
    geometry::Compartment comp2("comp2", {img}, col);
    REQUIRE(comp1.nVoxels() == 40);
    REQUIRE(comp2.nVoxels() == 2);

    geometry::Field field(&comp1, "field");
    REQUIRE(field.getCompartment() == &comp1);
    field.setUniformConcentration(2.0);
    REQUIRE(field.getConcentration().size() == comp1.nVoxels());

    // changing compartment to the same one is a no-op
    field.setCompartment(&comp1);

    // changing compartment: concentration reset to zero
    field.setCompartment(&comp2);
    REQUIRE(field.getCompartment() == &comp2);
    REQUIRE(field.getConcentration().size() == comp2.nVoxels());
    REQUIRE(field.getConcentration()[0] == dbl_approx(0.0));
    REQUIRE(field.getConcentration()[1] == dbl_approx(0.0));
  }
  SECTION("getConcentrationImageArray") {
    common::ImageStack img{
        {QImage(":/geometry/concave-cell-nucleus-100x100.png")}};
    QRgb col0 = img[0].pixel(0, 0);
    QRgb col1 = img[0].pixel(35, 20);
    QRgb col2 = img[0].pixel(40, 50);
    geometry::Compartment comp0("comp0", {img}, col0);
    geometry::Compartment comp1("comp1", {img}, col1);
    geometry::Compartment comp2("comp2", {img}, col2);
    geometry::Field field0(&comp0, "field0");
    geometry::Field field1(&comp1, "field1");
    geometry::Field field2(&comp2, "field2");
    for (std::size_t i = 0; i < field0.getCompartment()->nVoxels(); ++i) {
      field0.setConcentration(i, static_cast<double>(i) * 0.34);
    }
    for (std::size_t i = 0; i < field1.getCompartment()->nVoxels(); ++i) {
      field1.setConcentration(i, static_cast<double>(i) * 0.66);
    }
    for (std::size_t i = 0; i < field2.getCompartment()->nVoxels(); ++i) {
      field2.setConcentration(i, static_cast<double>(i) * 0.31 + 2.2);
    }
    auto a0{field0.getConcentrationImageArray()};
    auto t0{testGetConcentrationImageArray(field0)};
    REQUIRE(a0.size() == t0.size());
    for (std::size_t i = 0; i < a0.size(); ++i) {
      REQUIRE(a0[i] == dbl_approx(t0[i]));
    }
    auto a1{field1.getConcentrationImageArray()};
    auto t1{testGetConcentrationImageArray(field1)};
    REQUIRE(a1.size() == t1.size());
    for (std::size_t i = 0; i < a1.size(); ++i) {
      REQUIRE(a1[i] == dbl_approx(t1[i]));
    }
    auto a2{field2.getConcentrationImageArray()};
    auto t2{testGetConcentrationImageArray(field2)};
    REQUIRE(a2.size() == t2.size());
    for (std::size_t i = 0; i < a2.size(); ++i) {
      REQUIRE(a2[i] == dbl_approx(t2[i]));
    }
  }
  SECTION("3d fillMissingByDilation compartment with two disconnected voxels") {
    common::ImageStack imageStack({10, 10, 10}, QImage::Format_RGB32);
    QRgb col0 = 0xff000000;
    QRgb col1 = 0xff66aa00;
    imageStack.fill(col0);
    imageStack[5].setPixel(5, 5, col1);
    imageStack[3].setPixel(2, 1, col1);
    imageStack.convertToIndexed();
    geometry::Compartment compartment("c", imageStack, col1);
    REQUIRE(compartment.getVoxels().size() == 2);
    for (auto voxelIndex : compartment.getArrayPoints()) {
      // all voxels in image stack point to one of the 2 valid voxels in comp
      REQUIRE(voxelIndex <= 1);
    }
  }
  SECTION("3d fillMissingByDilation compartment with all voxels") {
    common::ImageStack imageStack({47, 53, 42}, QImage::Format_RGB32);
    auto nVoxels = imageStack.volume().nVoxels();
    QRgb col0 = 0xff000000;
    imageStack.fill(col0);
    imageStack.convertToIndexed();
    geometry::Compartment compartment("c", imageStack, col0);
    REQUIRE(compartment.getVoxels().size() == nVoxels);
    std::size_t nz = imageStack.volume().depth();
    int ny = imageStack.volume().height();
    int nx = imageStack.volume().width();
    for (std::size_t z = 0; z < nz; ++z) {
      for (int y = 0; y < ny; ++y) {
        for (int x = 0; x < nx; ++x) {
          auto i = static_cast<std::size_t>(x + nx * (ny - y - 1)) +
                   static_cast<std::size_t>(nx * ny) * z;
          // all voxels in image stack point to themselves
          auto v = compartment.getVoxel(compartment.getArrayPoints()[i]);
          REQUIRE(v.p.x() == x);
          REQUIRE(v.p.y() == y);
          REQUIRE(v.z == z);
        }
      }
    }
  }
}

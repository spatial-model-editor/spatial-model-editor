#include <QImage>
#include <QPoint>

#include "catch_wrapper.hpp"
#include "image_boundaries.hpp"

// note: only valid for vectors of unique values
static bool isCyclicPermutation(const std::vector<QPoint> &v1,
                                const std::vector<QPoint> &v2) {
  if (v1.size() != v2.size()) {
    // different size cannot be permutations
    return false;
  }
  auto iter = std::find(v1.cbegin(), v1.cend(), v2[0]);
  if (iter == v1.cend()) {
    // first element of v2 not found in v1
    return false;
  }
  // v1[i0] == v2[0]
  std::size_t i0 = static_cast<std::size_t>(iter - v1.cbegin());
  int di;
  if (v1[(i0 + 1) % v1.size()] == v2[1]) {
    // next elements going forwards match
    di = +1;
  } else if (v1[(i0 + v1.size() - 1) % v1.size()] == v2[1]) {
    // next elements going backwards match
    di = -1;
  } else {
    return false;
  }
  for (std::size_t i = 1; i < v1.size(); ++i) {
    // check all elements match
    i0 = static_cast<std::size_t>(static_cast<int>(i0 + v1.size()) + di) %
         v1.size();
    if (v1[i0] != v2[i]) {
      return false;
    }
  }
  return true;
}

SCENARIO("Image Boundaries",
         "[core/mesh/image_boundaries][core/mesh][core][image_boundaries]") {
  GIVEN("single compartment") {
    QImage img(24, 32, QImage::Format_RGB32);
    QRgb col = QColor(0, 0, 0).rgb();
    img.fill(col);
    // image boundary pixels
    std::vector<QPoint> imgBoundary{{0, 0}};
    imgBoundary.emplace_back(0, img.height() - 1);
    imgBoundary.emplace_back(img.width() - 1, img.height() - 1);
    imgBoundary.emplace_back(img.width() - 1, 0);
    std::vector<QRgb> compartmentColours{col};

    mesh::ImageBoundaries ib(img, compartmentColours);

    // check boundaries
    const auto &boundaries = ib.getBoundaries();
    REQUIRE(boundaries.size() == 1);
    REQUIRE(boundaries[0].isLoop());
    REQUIRE(isCyclicPermutation(boundaries[0].getPoints(), imgBoundary));
  }
  GIVEN("two compartments separated by membrane") {
    QImage img(24, 32, QImage::Format_RGB32);
    QRgb col1 = qRgb(0, 255, 0);
    QRgb col2 = qRgb(0, 0, 255);
    for (int y = 0; y < img.height(); ++y) {
      for (int x = 0; x < img.width() / 2; ++x) {
        img.setPixel(x, y, col1);
      }
      for (int x = img.width() / 2; x < img.width(); ++x) {
        img.setPixel(x, y, col2);
      }
    }
    std::vector<QRgb> compartmentColours{col1, col2};
    std::vector<std::pair<std::string, mesh::ColourPair>> membranes = {
        {"membrane", {col1, col2}}};
    mesh::ImageBoundaries ib(img, compartmentColours, membranes);

    // check boundaries
    const auto &boundaries = ib.getBoundaries();
    const auto &fp = ib.getFixedPoints();
    REQUIRE(boundaries.size() == 3);
    REQUIRE(fp.size() == 2);
    REQUIRE(fp[0].point == QPoint(11, 31));
    REQUIRE(fp[1].point == QPoint(11, 0));
    const auto &bLeft = boundaries[0];
    const auto &bMembrane = boundaries[1];
    const auto &bRight = boundaries[2];
    REQUIRE(bLeft.isLoop() == false);
    REQUIRE(bLeft.isMembrane() == false);
    REQUIRE(bLeft.getFpIndices().startPoint == 0);
    REQUIRE(bLeft.getFpIndices().endPoint == 1);
    REQUIRE(bLeft.getPoints().size() == 4);
    REQUIRE(bMembrane.isLoop() == false);
    REQUIRE(bMembrane.isMembrane() == true);
    REQUIRE(bMembrane.getMembraneId() == "membrane");
    REQUIRE(bMembrane.getFpIndices().startPoint == 0);
    REQUIRE(bMembrane.getFpIndices().endPoint == 1);
    REQUIRE(bMembrane.getPoints().size() == 2);
    REQUIRE(bRight.isLoop() == false);
    REQUIRE(bRight.isMembrane() == false);
    REQUIRE(bRight.getFpIndices().startPoint == 0);
    REQUIRE(bRight.getFpIndices().endPoint == 1);
    REQUIRE(bRight.getPoints().size() == 4);
    // automatic determination of sensible number of points for each boundary
    std::vector<std::size_t> autoMaxPoints = {4, 2, 4};
    REQUIRE(ib.setAutoMaxPoints() == autoMaxPoints);
  }
}

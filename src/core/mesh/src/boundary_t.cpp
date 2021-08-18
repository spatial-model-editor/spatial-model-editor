#include "boundary.hpp"
#include "catch_wrapper.hpp"
#include "interior_point.hpp"
#include "utils.hpp"
#include <QImage>
#include <QPoint>
#include <cmath>

struct BoundaryTestData {
  std::vector<QRgb> colours;
  std::size_t nBoundaries;
  std::size_t nLoops;
  std::vector<std::size_t> nInteriorPoints;
};

struct BoundaryTestImage {
  std::string description;
  std::vector<int> images;
  std::vector<BoundaryTestData> boundaries;
};

using namespace sme;

SCENARIO("Boundary", "[core/mesh/boundary][core/mesh][core][boundary]") {
  GIVEN("Loop") {
    // points that make up the loop
    std::vector<QPoint> points{{0, 0}, {2, 0},  {3, 1}, {1, 3},
                               {0, 3}, {-1, 2}, {-1, 1}};
    // 3-point approximation to loop
    std::vector<QPoint> p3{
        {0, 0},
        {3, 1},
        {1, 3},
    };
    // 4-point approximation to loop
    std::vector<QPoint> p4{
        {0, 0},
        {3, 1},
        {1, 3},
        {-1, 2},
    };
    mesh::Boundary boundary(points, true);
    REQUIRE(boundary.isValid() == true);
    REQUIRE(boundary.isLoop() == true);
    // use all points
    boundary.setMaxPoints(999);
    REQUIRE(boundary.getMaxPoints() == 999);
    REQUIRE(boundary.getPoints() == points);
    // use 3 points for boundary
    boundary.setMaxPoints(3);
    REQUIRE(boundary.getMaxPoints() == 3);
    REQUIRE(boundary.getPoints() == p3);
    // automatically determine number of points
    auto nPoints = boundary.setMaxPoints();
    REQUIRE(boundary.getMaxPoints() == nPoints);
    REQUIRE(nPoints == 4);
    REQUIRE(boundary.getPoints() == p4);
  }
  GIVEN("Non-loop") {
    // points that make up the boundary line
    std::vector<QPoint> points{{0, 0}, {2, 0}, {3, 1}, {4, 1},
                               {4, 2}, {5, 3}, {5, 4}};
    // 4-point approximation to line
    std::vector<QPoint> p4{{0, 0}, {2, 0}, {4, 1}, {5, 4}};
    // 3-point approximation to line
    std::vector<QPoint> p3{{0, 0}, {4, 1}, {5, 4}};
    mesh::Boundary boundary(points, false);
    REQUIRE(boundary.isValid() == true);
    REQUIRE(boundary.isLoop() == false);
    // use all points
    boundary.setMaxPoints(999);
    REQUIRE(boundary.getMaxPoints() == 999);
    REQUIRE(boundary.getPoints() == points);
    // use 3 points for boundary
    boundary.setMaxPoints(4);
    REQUIRE(boundary.getMaxPoints() == 4);
    REQUIRE(boundary.getPoints() == p4);
    // automatically determine number of points
    auto nPoints = boundary.setMaxPoints();
    REQUIRE(boundary.getMaxPoints() == nPoints);
    REQUIRE(nPoints == 3);
    REQUIRE(boundary.getPoints() == p3);
  }
}

SCENARIO("constructBoundaries",
         "[core/mesh/boundary][core/mesh][core][boundary]") {
  GIVEN("empty image") {
    QImage img(24, 32, QImage::Format_RGB32);
    QRgb col = qRgb(0, 0, 0);
    img.fill(col);
    WHEN("no compartment colours") {
      auto interiorPoints{mesh::getInteriorPoints(img, {})};
      auto boundaries = mesh::constructBoundaries(img, {});
      REQUIRE(boundaries.empty());
      REQUIRE(interiorPoints.empty());
    }
    WHEN("whole image is a compartment") {
      auto interiorPoints{mesh::getInteriorPoints(img, {col})};
      auto boundaries = mesh::constructBoundaries(img, {col});
      std::vector<QPoint> outer{{0, 0},
                                {img.width(), 0},
                                {img.width(), img.height()},
                                {0, img.height()}};
      REQUIRE(boundaries.size() == 1);
      REQUIRE(interiorPoints.size() == 1);
      REQUIRE(interiorPoints[0].size() == 1);
      auto point{interiorPoints.front().front()};
      REQUIRE(point.x() == dbl_approx(11.5));
      REQUIRE(point.y() == dbl_approx(20.5));
      const auto &b = boundaries[0];
      REQUIRE(b.isValid());
      REQUIRE(b.isLoop());
      REQUIRE(utils::isCyclicPermutation(b.getAllPoints(), outer));
    }
  }
  GIVEN("image with 3 concentric compartments, no fixed points") {
    QImage img(":/geometry/concave-cell-nucleus-100x100.png");
    QRgb col0 = img.pixel(0, 0);
    QRgb col1 = img.pixel(35, 20);
    QRgb col2 = img.pixel(40, 50);
    auto interiorPoints{mesh::getInteriorPoints(img, {col0, col1, col2})};
    auto boundaries = mesh::constructBoundaries(img, {col0, col1, col2});

    REQUIRE(boundaries.size() == 3);
    REQUIRE(interiorPoints.size() == 3);
    REQUIRE(interiorPoints[0].size() == 1);
    REQUIRE(interiorPoints[1].size() == 1);
    REQUIRE(interiorPoints[2].size() == 1);

    const auto &b0 = boundaries[0];
    REQUIRE(b0.isValid());
    REQUIRE(b0.isLoop());

    const auto &b1 = boundaries[1];
    REQUIRE(b1.isValid());
    REQUIRE(b1.isLoop());

    const auto &b2 = boundaries[2];
    REQUIRE(b2.isValid());
    REQUIRE(b2.isLoop());
  }
  GIVEN("image with 3 compartments, two fixed points") {
    QImage img(":/geometry/two-blobs-100x100.png");
    QRgb col0 = img.pixel(0, 0);
    QRgb col1 = img.pixel(33, 33);
    QRgb col2 = img.pixel(66, 66);
    auto interiorPoints{mesh::getInteriorPoints(img, {col0, col1, col2})};
    auto boundaries = mesh::constructBoundaries(img, {col0, col1, col2});

    REQUIRE(boundaries.size() == 4);
    REQUIRE(interiorPoints.size() == 3);
    REQUIRE(interiorPoints[0].size() == 1);
    REQUIRE(interiorPoints[1].size() == 1);
    REQUIRE(interiorPoints[2].size() == 1);

    const auto &b0 = boundaries[0];
    REQUIRE(b0.isValid());
    REQUIRE(b0.isLoop());

    const auto &b1 = boundaries[1];
    REQUIRE(b1.isValid());
    REQUIRE(!b1.isLoop());

    const auto &b2 = boundaries[2];
    REQUIRE(b2.isValid());
    REQUIRE(!b2.isLoop());

    const auto &b3 = boundaries[3];
    REQUIRE(b3.isValid());
    REQUIRE(!b3.isLoop());
  }
}

SCENARIO("constructBoundaries Medha Bhattacharya images",
         "[core/mesh/boundary][core/mesh][core][boundary][expensive]") {
  // images provided by Medha Bhattacharya
  //  https://summerofcode.withgoogle.com/archive/2020/projects/4913136240427008/
  // https://drive.google.com/drive/folders/1z83_pUTlI7eYKL9J9iV-EV6lLAEqC7pG
  QRgb bg{qRgb(0, 0, 0)};
  QRgb fg{qRgb(255, 255, 255)};
  std::vector<BoundaryTestImage> boundaryTestImages;

  auto &bt0 = boundaryTestImages.emplace_back();
  bt0.description = "single cell, separated from edges of image";
  bt0.images = {{3, 8, 11, 12, 13, 18, 19, 20}};
  bt0.boundaries.push_back({{bg}, 2, 2, {1}});
  bt0.boundaries.push_back({{fg}, 1, 1, {1}});
  bt0.boundaries.push_back({{bg, fg}, 2, 2, {1, 1}});

  auto &bt1 = boundaryTestImages.emplace_back();
  bt1.description = "single cell, one boundary touches edge of image";
  bt1.images = {{1, 5, 9, 16}};
  bt1.boundaries.push_back({{bg}, 1, 1, {1}});
  bt1.boundaries.push_back({{fg}, 1, 1, {1}});
  bt1.boundaries.push_back({{bg, fg}, 3, 0, {1, 1}});

  auto &bt2 = boundaryTestImages.emplace_back();
  bt2.description = "two cells, one touching edge of image";
  bt2.images = {4, 7, 10};
  bt2.boundaries.push_back({{bg}, 2, 2, {1}});
  bt2.boundaries.push_back({{fg}, 2, 2, {2}});
  bt2.boundaries.push_back({{bg, fg}, 4, 1, {1, 2}});

  auto &bt3 = boundaryTestImages.emplace_back();
  bt3.description = "two cells, both touching edge of image";
  bt3.images = {14};
  bt3.boundaries.push_back({{bg}, 1, 1, {1}});
  bt3.boundaries.push_back({{fg}, 2, 2, {2}});
  bt3.boundaries.push_back({{bg, fg}, 6, 0, {1, 2}});

  auto &bt4 = boundaryTestImages.emplace_back();
  bt4.description = "three cells, each with a boundary touching edge of image";
  bt4.images = {{6, 15}};
  bt4.boundaries.push_back({{bg}, 1, 1, {1}});
  bt4.boundaries.push_back({{fg}, 3, 3, {3}});
  bt4.boundaries.push_back({{bg, fg}, 9, 0, {1, 3}});

  auto &bt5 = boundaryTestImages.emplace_back();
  bt5.description = "one fg cell, three bg cells";
  bt5.images = {{2, 17}};
  bt5.boundaries.push_back({{bg}, 3, 3, {3}});
  bt5.boundaries.push_back({{fg}, 1, 1, {1}});
  bt5.boundaries.push_back({{bg, fg}, 9, 0, {3, 1}});

  for (const auto &boundaryTestImage : boundaryTestImages) {
    for (auto imageIndex : boundaryTestImage.images) {
      for (const auto &boundaryData : boundaryTestImage.boundaries) {
        QString filename = QString(":/test/geometry/gt%1.png").arg(imageIndex);
        CAPTURE(boundaryTestImage.description);
        CAPTURE(filename.toStdString());
        QImage img(filename);
        auto interiorPoints{mesh::getInteriorPoints(img, boundaryData.colours)};
        auto boundaries = mesh::constructBoundaries(img, boundaryData.colours);
        REQUIRE(boundaries.size() == boundaryData.nBoundaries);
        REQUIRE(interiorPoints.size() == boundaryData.colours.size());
        for (std::size_t i = 0; i < interiorPoints.size(); ++i) {
          CAPTURE(i);
          REQUIRE(interiorPoints[i].size() == boundaryData.nInteriorPoints[i]);
        }
        std::size_t nValid{0};
        std::size_t nLoops{0};
        for (const auto &boundary : boundaries) {
          nValid += static_cast<std::size_t>(boundary.isValid());
          nLoops += static_cast<std::size_t>(boundary.isLoop());
        }
        REQUIRE(nValid == boundaryData.nBoundaries);
        REQUIRE(nLoops == boundaryData.nLoops);
      }
    }
  }
}

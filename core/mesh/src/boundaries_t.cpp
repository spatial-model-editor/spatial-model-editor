#include "boundaries.hpp"
#include "catch_wrapper.hpp"
#include "interior_point.hpp"
#include "sme/utils.hpp"
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

TEST_CASE("Boundaries", "[core/mesh/boundaries][core/mesh][core][boundaries]") {
  SECTION("empty image") {
    QImage img(24, 32, QImage::Format_RGB32);
    QRgb col = qRgb(0, 0, 0);
    img.fill(col);
    SECTION("no compartment colours") {
      auto interiorPoints{mesh::getInteriorPoints(img, {})};
      mesh::Boundaries boundaries0(img, {}, 0);
      REQUIRE(boundaries0.getSimplifierType() == 0);
      REQUIRE(boundaries0.size() == 0);
      REQUIRE(interiorPoints.empty());
      mesh::Boundaries boundaries1(img, {}, 1);
      REQUIRE(boundaries1.getSimplifierType() == 1);
      REQUIRE(boundaries1.size() == 0);
      boundaries1.setSimplifierType(0);
      REQUIRE(boundaries1.getSimplifierType() == 0);
    }
    SECTION("whole image is a compartment") {
      auto interiorPoints{mesh::getInteriorPoints(img, {col})};
      mesh::Boundaries boundaries(img, {col}, 0);
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
      const auto &b = boundaries.getBoundaries()[0];
      REQUIRE(b.isValid());
      REQUIRE(b.isLoop());
      REQUIRE(common::isCyclicPermutation(b.getAllPoints(), outer));
      REQUIRE(boundaries.getSimplifierType() == 0);
      REQUIRE(boundaries.getBoundaries().size() == 1);
      // set too many points for max
      boundaries.setMaxPoints(0, 8);
      REQUIRE(boundaries.getMaxPoints(0) == 8);
      // set too few points for max
      boundaries.setMaxPoints(0, 1);
      REQUIRE(boundaries.getMaxPoints(0) == 1);
      // automatic max points
      boundaries.setMaxPoints();
      REQUIRE(boundaries.getMaxPoints(0) == 4);
      // change to topology-preserving line simplification
      boundaries.setSimplifierType(1);
      REQUIRE(boundaries.getSimplifierType() == 1);
      // set too many points for max
      boundaries.setMaxPoints(8);
      REQUIRE(boundaries.getMaxPoints()[0] == 4);
      // set too few points for max
      boundaries.setMaxPoints(1);
      REQUIRE(boundaries.getMaxPoints()[0] == 3);
      // setting existing number of points is a no-op
      boundaries.setMaxPoints(3);
      REQUIRE(boundaries.getMaxPoints()[0] == 3);
      // automatic max points
      boundaries.setMaxPoints();
      REQUIRE(boundaries.getMaxPoints()[0] == 4);
    }
  }
  SECTION("image with 3 concentric compartments, no fixed points") {
    QImage img(":/geometry/concave-cell-nucleus-100x100.png");
    QRgb col0 = img.pixel(0, 0);
    QRgb col1 = img.pixel(35, 20);
    QRgb col2 = img.pixel(40, 50);
    auto interiorPoints{mesh::getInteriorPoints(img, {col0, col1, col2})};
    mesh::Boundaries boundaries(img, {col0, col1, col2}, 0);

    REQUIRE(boundaries.size() == 3);
    REQUIRE(interiorPoints.size() == 3);
    REQUIRE(interiorPoints[0].size() == 1);
    REQUIRE(interiorPoints[1].size() == 1);
    REQUIRE(interiorPoints[2].size() == 1);

    const auto &b0 = boundaries.getBoundaries()[0];
    REQUIRE(b0.isValid());
    REQUIRE(b0.isLoop());

    const auto &b1 = boundaries.getBoundaries()[1];
    REQUIRE(b1.isValid());
    REQUIRE(b1.isLoop());

    const auto &b2 = boundaries.getBoundaries()[2];
    REQUIRE(b2.isValid());
    REQUIRE(b2.isLoop());
  }
  SECTION("image with 3 compartments, two fixed points") {
    QImage img(":/geometry/two-blobs-100x100.png");
    QRgb col0 = img.pixel(0, 0);
    QRgb col1 = img.pixel(33, 33);
    QRgb col2 = img.pixel(66, 66);
    auto interiorPoints{mesh::getInteriorPoints(img, {col0, col1, col2})};
    mesh::Boundaries boundaries(img, {col0, col1, col2}, 1);

    REQUIRE(boundaries.size() == 4);
    REQUIRE(interiorPoints.size() == 3);
    REQUIRE(interiorPoints[0].size() == 1);
    REQUIRE(interiorPoints[1].size() == 1);
    REQUIRE(interiorPoints[2].size() == 1);

    const auto &b0 = boundaries.getBoundaries()[0];
    REQUIRE(b0.isValid());
    REQUIRE(b0.isLoop());

    const auto &b1 = boundaries.getBoundaries()[1];
    REQUIRE(b1.isValid());
    REQUIRE(!b1.isLoop());

    const auto &b2 = boundaries.getBoundaries()[2];
    REQUIRE(b2.isValid());
    REQUIRE(!b2.isLoop());

    const auto &b3 = boundaries.getBoundaries()[3];
    REQUIRE(b3.isValid());
    REQUIRE(!b3.isLoop());
  }
}

TEST_CASE("Boundaries using images from Medha Bhattacharya",
          "[core/mesh/boundaries][core/mesh][core][boundaries][expensive]") {
  // images provided by Medha Bhattacharya
  // https://summerofcode.withgoogle.com/archive/2020/projects/4913136240427008/
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
        mesh::Boundaries boundaries(img, boundaryData.colours, 0);
        REQUIRE(boundaries.size() == boundaryData.nBoundaries);
        REQUIRE(interiorPoints.size() == boundaryData.colours.size());
        for (std::size_t i = 0; i < interiorPoints.size(); ++i) {
          CAPTURE(i);
          REQUIRE(interiorPoints[i].size() == boundaryData.nInteriorPoints[i]);
        }
        std::size_t nValid{0};
        std::size_t nLoops{0};
        for (const auto &boundary : boundaries.getBoundaries()) {
          nValid += static_cast<std::size_t>(boundary.isValid());
          nLoops += static_cast<std::size_t>(boundary.isLoop());
        }
        REQUIRE(nValid == boundaryData.nBoundaries);
        REQUIRE(nLoops == boundaryData.nLoops);
      }
    }
  }
}

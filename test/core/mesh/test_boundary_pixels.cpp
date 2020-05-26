#include <QImage>
#include <QPoint>

#include "boundary_pixels.hpp"
#include "catch_wrapper.hpp"

SCENARIO("BoundaryPixels", "[core][mesh][boundary_pixels]") {
  GIVEN("empty image") {
    QImage img(24, 32, QImage::Format_RGB32);
    QRgb col = qRgb(0, 0, 0);
    img.fill(col);
    WHEN("no compartment colours") {
      mesh::BoundaryPixels bpx(img, {}, {});
      REQUIRE(bpx.width() == img.width());
      REQUIRE(bpx.height() == img.height());
      REQUIRE(bpx.isValid(QPoint(-1, -1)) == false);
      REQUIRE(bpx.isValid(QPoint(23, 31)) == true);
      REQUIRE(bpx.isValid(QPoint(24, 31)) == false);
      REQUIRE(bpx.isValid(QPoint(23, 32)) == false);
      const auto& imgPixels = bpx.getBoundaryPixelsImage();
      // no pixels are boundary points
      for (const auto& p : {QPoint(4, 5), QPoint(19, 19), QPoint(0, 0)}) {
        REQUIRE(bpx.isValid(p) == true);
        REQUIRE(bpx.isBoundary(p) == false);
        REQUIRE(bpx.isMembrane(p) == false);
        REQUIRE(bpx.isFixed(p) == false);
        REQUIRE(imgPixels.pixel(p) == qRgba(0, 0, 0, 0));
      }
    }
    WHEN("whole image is a compartment") {
      mesh::BoundaryPixels bpx(img, {col}, {});
      REQUIRE(bpx.width() == img.width());
      REQUIRE(bpx.height() == img.height());
      REQUIRE(bpx.isValid(QPoint(-1, -1)) == false);
      REQUIRE(bpx.isValid(QPoint(23, 31)) == true);
      REQUIRE(bpx.isValid(QPoint(24, 31)) == false);
      REQUIRE(bpx.isValid(QPoint(23, 32)) == false);
      auto& imgPixels = bpx.getBoundaryPixelsImage();
      // inner pixels are nothing
      for (int x = 1; x < img.width() - 1; ++x) {
        for (int y = 1; y < img.height() - 1; ++y) {
          QPoint p(x, y);
          REQUIRE(bpx.isValid(p) == true);
          REQUIRE(bpx.isBoundary(p) == false);
          REQUIRE(bpx.isMembrane(p) == false);
          REQUIRE(bpx.isFixed(p) == false);
          REQUIRE(imgPixels.pixel(p) == qRgba(0, 0, 0, 0));
        }
      }
      // edge of image pixels are boundaries
      for (int x = 0; x < img.width(); ++x) {
        for (int y : {0, img.height() - 1}) {
          QPoint p(x, y);
          REQUIRE(bpx.isValid(p) == true);
          REQUIRE(bpx.isBoundary(p) == true);
          REQUIRE(bpx.isMembrane(p) == false);
          REQUIRE(bpx.isFixed(p) == false);
          REQUIRE(imgPixels.pixel(p) == qRgb(0, 0, 0));
        }
      }
      for (int y = 0; y < img.height(); ++y) {
        for (int x : {0, img.width() - 1}) {
          QPoint p(x, y);
          REQUIRE(bpx.isValid(p) == true);
          REQUIRE(bpx.isBoundary(p) == true);
          REQUIRE(bpx.isMembrane(p) == false);
          REQUIRE(bpx.isFixed(p) == false);
          REQUIRE(imgPixels.pixel(p) == qRgb(0, 0, 0));
        }
      }
    }
  }
  GIVEN("image with 3 concentric compartments, no fixed points") {
    QImage img(":/geometry/concave-cell-nucleus-100x100.png");
    QRgb col0 = img.pixel(0, 0);
    QRgb col1 = img.pixel(35, 20);
    QRgb col2 = img.pixel(40, 50);
    std::vector<QRgb> compartments{{col0, col1, col2}};
    std::vector<std::pair<std::string, mesh::ColourPair>> membranes{
        {"outside-cell", {col0, col1}}, {"cell-nucleus", {col1, col2}}};
    mesh::BoundaryPixels bpx(img, compartments, membranes);
    REQUIRE(bpx.width() == img.width());
    REQUIRE(bpx.height() == img.height());

    // non-boundary point
    QPoint p(89, 49);
    REQUIRE(bpx.isValid(p) == true);
    REQUIRE(bpx.isBoundary(p) == false);
    REQUIRE(bpx.isMembrane(p) == false);
    REQUIRE(bpx.getNeighbourOnBoundary(p).has_value());
    REQUIRE(bpx.isFixed(p) == false);

    // boundary point
    p = QPoint(0, 0);
    REQUIRE(bpx.isValid(p) == true);
    REQUIRE(bpx.isBoundary(p) == true);
    REQUIRE(bpx.getNeighbourOnBoundary(p).has_value());
    REQUIRE(bpx.isMembrane(p) == false);
    REQUIRE(bpx.isFixed(p) == false);
    bpx.visitPoint(p);
    REQUIRE(bpx.isValid(p) == true);
    REQUIRE(bpx.isBoundary(p) == false);

    // membrane boundary point
    p = QPoint(10, 49);
    REQUIRE(bpx.isValid(p) == true);
    REQUIRE(bpx.isBoundary(p) == true);
    REQUIRE(bpx.getNeighbourOnBoundary(p).has_value());
    auto pNeighbour = bpx.getNeighbourOnBoundary(p).value();
    REQUIRE(bpx.getMembraneIndex(pNeighbour) == bpx.getMembraneIndex(p));
    REQUIRE(bpx.isMembrane(p) == true);
    REQUIRE(bpx.getMembraneIndex(p) == 0);
    REQUIRE(bpx.getMembraneName(bpx.getMembraneIndex(p)) == "outside-cell");
    REQUIRE(bpx.isFixed(p) == false);

    // membrane boundary point
    p = QPoint(32, 60);
    REQUIRE(bpx.isValid(p) == true);
    REQUIRE(bpx.isBoundary(p) == true);
    REQUIRE(bpx.getNeighbourOnBoundary(p).has_value());
    pNeighbour = bpx.getNeighbourOnBoundary(p).value();
    REQUIRE(bpx.getMembraneIndex(pNeighbour) == bpx.getMembraneIndex(p));
    REQUIRE(bpx.isMembrane(p) == true);
    REQUIRE(bpx.getMembraneIndex(p) == 1);
    REQUIRE(bpx.getMembraneName(bpx.getMembraneIndex(p)) == "cell-nucleus");
    REQUIRE(bpx.isFixed(p) == false);
  }
  GIVEN("image with 3 compartments, two fixed points") {
    QImage img(":/geometry/two-blobs-100x100.png");
    QRgb col0 = img.pixel(0, 0);
    QRgb col1 = img.pixel(33, 33);
    QRgb col2 = img.pixel(66, 66);
    std::vector<QRgb> compartments{{col0, col1, col2}};
    std::vector<std::pair<std::string, mesh::ColourPair>> membranes{
        {"membrane", {col1, col2}}};
    mesh::BoundaryPixels bpx(img, compartments, membranes);
    REQUIRE(bpx.width() == img.width());
    REQUIRE(bpx.height() == img.height());

    // non-boundary point
    QPoint p(89, 49);
    REQUIRE(bpx.isValid(p) == true);
    REQUIRE(bpx.isBoundary(p) == false);
    REQUIRE(bpx.isMembrane(p) == false);
    REQUIRE(bpx.getNeighbourOnBoundary(p).has_value());
    REQUIRE(bpx.isFixed(p) == false);

    // boundary point
    p = QPoint(0, 0);
    REQUIRE(bpx.isValid(p) == true);
    REQUIRE(bpx.isBoundary(p) == true);
    REQUIRE(bpx.getNeighbourOnBoundary(p).has_value());
    REQUIRE(bpx.isMembrane(p) == false);
    REQUIRE(bpx.isFixed(p) == false);

    // boundary point
    p = QPoint(14, 66);
    REQUIRE(bpx.isValid(p) == true);
    REQUIRE(bpx.isBoundary(p) == true);
    REQUIRE(bpx.getNeighbourOnBoundary(p).has_value());
    REQUIRE(bpx.isMembrane(p) == false);
    REQUIRE(bpx.isFixed(p) == false);

    // boundary point
    p = QPoint(29, 33);
    REQUIRE(bpx.isValid(p) == true);
    REQUIRE(bpx.isBoundary(p) == true);
    REQUIRE(bpx.getNeighbourOnBoundary(p).has_value());
    REQUIRE(bpx.isMembrane(p) == false);
    REQUIRE(bpx.isFixed(p) == false);

    // membrane boundary point
    p = QPoint(51, 59);
    REQUIRE(bpx.isValid(p) == true);
    REQUIRE(bpx.isBoundary(p) == true);
    REQUIRE(bpx.getNeighbourOnBoundary(p).has_value());
    auto pNeighbour = bpx.getNeighbourOnBoundary(p).value();
    REQUIRE(bpx.getMembraneIndex(pNeighbour) == bpx.getMembraneIndex(p));
    REQUIRE(bpx.isMembrane(p) == true);
    REQUIRE(bpx.getMembraneIndex(p) == 0);
    REQUIRE(bpx.getMembraneName(bpx.getMembraneIndex(p)) == "membrane");
    REQUIRE(bpx.isFixed(p) == false);

    // fixed boundary point
    QPoint fp = QPoint(54, 60);
    REQUIRE(bpx.isValid(fp) == true);
    REQUIRE(bpx.isBoundary(fp) == true);
    REQUIRE(bpx.isMembrane(fp) == false);
    REQUIRE(bpx.isFixed(fp) == true);
    REQUIRE(bpx.getFixedPoint(fp) == fp);
    REQUIRE(bpx.getFixedPointIndex(fp) == 1);
    REQUIRE(bpx.getFixedPoints()[1] == fp);

    // another point within this fp
    p = QPoint(55, 60);
    REQUIRE(bpx.isValid(p) == true);
    REQUIRE(bpx.isBoundary(p) == true);
    REQUIRE(bpx.isMembrane(p) == false);
    REQUIRE(bpx.isFixed(p) == true);
    REQUIRE(bpx.getFixedPoint(p) == fp);
    REQUIRE(bpx.getFixedPointIndex(p) == 1);
  }
}

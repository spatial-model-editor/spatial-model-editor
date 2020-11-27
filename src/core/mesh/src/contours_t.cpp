#include "catch_wrapper.hpp"
#include "contours.hpp"
#include "utils.hpp"
#include <QImage>
#include <QPoint>

SCENARIO("Contours", "[core/mesh/contours][core/mesh][core][contours]") {
  GIVEN("empty image") {
    QImage img(24, 32, QImage::Format_RGB32);
    QRgb col = qRgb(0, 0, 0);
    img.fill(col);
    WHEN("no compartment colours") {
      mesh::Contours c(img, {}, {});
      REQUIRE(c.getBoundaries().empty());
      REQUIRE(c.getFixedPoints().empty());
      const auto &imgPixels = c.getBoundaryPixelsImage();
      // no pixels are boundary points
      for (const auto &p : {QPoint(4, 5), QPoint(19, 19), QPoint(0, 0)}) {
        REQUIRE(imgPixels.pixel(p) == qRgba(0, 0, 0, 0));
      }
    }
    WHEN("whole image is a compartment") {
      mesh::Contours c(img, {col}, {});
      REQUIRE(c.getBoundaries().size() == 1);
      REQUIRE(c.getFixedPoints().empty());
      const auto &b = c.getBoundaries()[0];
      REQUIRE(b.isValid());
      REQUIRE(b.isLoop());
      REQUIRE(!b.isMembrane());
      auto &imgPixels = c.getBoundaryPixelsImage();
      REQUIRE(imgPixels.size() == img.size());
      // inner pixels are nothing
      for (int x = 1; x < img.width() - 1; ++x) {
        for (int y = 1; y < img.height() - 1; ++y) {
          QPoint p(x, y);
          REQUIRE(imgPixels.pixel(p) == qRgba(0, 0, 0, 0));
        }
      }
      // edge of image pixels forms single loop boundary
      auto c0 = utils::indexedColours()[0].rgb();
      for (int x = 0; x < img.width(); ++x) {
        for (int y : {0, img.height() - 1}) {
          QPoint p(x, y);
          REQUIRE(imgPixels.pixel(p) == c0);
        }
      }
      for (int y = 0; y < img.height(); ++y) {
        for (int x : {0, img.width() - 1}) {
          QPoint p(x, y);
          REQUIRE(imgPixels.pixel(p) == c0);
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
    mesh::Contours c(img, compartments, membranes);

    // boundaries
    REQUIRE(c.getBoundaries().size() == 3);
    REQUIRE(c.getFixedPoints().empty());

    const auto &b0 = c.getBoundaries()[0];
    REQUIRE(b0.isValid());
    REQUIRE(b0.isLoop());
    REQUIRE(b0.isMembrane());

    const auto &b1 = c.getBoundaries()[1];
    REQUIRE(b1.isValid());
    REQUIRE(b1.isLoop());
    REQUIRE(!b1.isMembrane());

    const auto &b2 = c.getBoundaries()[2];
    REQUIRE(b2.isValid());
    REQUIRE(b2.isLoop());
    REQUIRE(b2.isMembrane());

    // image
    auto &imgPixels = c.getBoundaryPixelsImage();
    REQUIRE(imgPixels.size() == img.size());
    // non-boundary point
    QPoint p(89, 49);
    REQUIRE(imgPixels.pixel(p) == qRgba(0, 0, 0, 0));
    // outer boundary point
    p = QPoint(0, 0);
    REQUIRE(imgPixels.pixel(p) == utils::indexedColours()[1].rgb());
    // outer membrane boundary point
    p = QPoint(10, 49);
    REQUIRE(imgPixels.pixel(p) == utils::indexedColours()[0].rgb());
    // inner membrane boundary point
    p = QPoint(31, 60);
    REQUIRE(imgPixels.pixel(p) == utils::indexedColours()[2].rgb());
  }
  GIVEN("image with 3 compartments, two fixed points") {
    QImage img(":/geometry/two-blobs-100x100.png");
    QRgb col0 = img.pixel(0, 0);
    QRgb col1 = img.pixel(33, 33);
    QRgb col2 = img.pixel(66, 66);
    std::vector<QRgb> compartments{{col0, col1, col2}};
    std::vector<std::pair<std::string, mesh::ColourPair>> membranes{
        {"membrane", {col1, col2}}};
    mesh::Contours c(img, compartments, membranes);

    // boundaries
    REQUIRE(c.getBoundaries().size() == 4);
    REQUIRE(c.getFixedPoints().size() == 2);

    const auto &b0 = c.getBoundaries()[0];
    REQUIRE(b0.isValid());
    REQUIRE(b0.isLoop());
    REQUIRE(!b0.isMembrane());

    const auto &b1 = c.getBoundaries()[1];
    REQUIRE(b1.isValid());
    REQUIRE(!b1.isLoop());
    REQUIRE(!b1.isMembrane());

    const auto &b2 = c.getBoundaries()[2];
    REQUIRE(b2.isValid());
    REQUIRE(!b2.isLoop());
    REQUIRE(!b2.isMembrane());

    const auto &b3 = c.getBoundaries()[3];
    REQUIRE(b3.isValid());
    REQUIRE(!b3.isLoop());
    REQUIRE(b3.isMembrane());

    const auto &fp0 = c.getFixedPoints()[0];
    REQUIRE(fp0.point == QPoint(33, 42));
    REQUIRE(fp0.lines.size() == 3);

    const auto &fp1 = c.getFixedPoints()[1];
    REQUIRE(fp1.point == QPoint(55, 60));
    REQUIRE(fp1.lines.size() == 3);

    // image
    auto &imgPixels = c.getBoundaryPixelsImage();
    REQUIRE(imgPixels.size() == img.size());
    // non-boundary point
    QPoint p(89, 49);
    REQUIRE(imgPixels.pixel(p) == qRgba(0, 0, 0, 0));
    // boundary point
    p = QPoint(0, 0);
    REQUIRE(imgPixels.pixel(p) == utils::indexedColours()[0].rgb());
    // boundary point
    p = QPoint(14, 35);
    REQUIRE(imgPixels.pixel(p) == utils::indexedColours()[1].rgb());
    // boundary point
    p = QPoint(89, 67);
    REQUIRE(imgPixels.pixel(p) == utils::indexedColours()[2].rgb());
    // membrane boundary point
    p = QPoint(49, 41);
    REQUIRE(imgPixels.pixel(p) == utils::indexedColours()[3].rgb());
  }
  GIVEN("single cell not too close to boundary") {
    QStringList filenames{"gt3.png",  "gt11.png", "gt12.png", "gt13.png",
                          "gt18.png", "gt19.png", "gt20.png"};
    for (const auto &filename : filenames) {
      CAPTURE(filename.toStdString());
      QImage img(QString(":/test/geometry/%1").arg(filename));

      // single compartment: black (outer)
      // inner and outer loop boundaries
      mesh::Contours cBlack(img, {qRgb(0, 0, 0)}, {});
      REQUIRE(cBlack.getBoundaries().size() == 2);
      REQUIRE(cBlack.getFixedPoints().empty());
      for (const auto &b : cBlack.getBoundaries()) {
        REQUIRE(b.isValid());
        REQUIRE(b.isLoop());
        REQUIRE(!b.isMembrane());
      }

      // single compartment: white (inner)
      // single outer loop boundary
      mesh::Contours cWhite(img, {qRgb(255, 255, 255)}, {});
      REQUIRE(cWhite.getBoundaries().size() == 1);
      REQUIRE(cWhite.getFixedPoints().empty());
      const auto &bWhite = cWhite.getBoundaries()[0];
      REQUIRE(bWhite.isValid());
      REQUIRE(bWhite.isLoop());
      REQUIRE(!bWhite.isMembrane());

      // two compartments: black and white
      // outer loop boundary, inner membrane loop boundary
      QRgb col0 = qRgb(0, 0, 0);
      QRgb col1 = qRgb(255, 255, 255);
      std::vector<QRgb> compartments{{col0, col1}};
      std::vector<std::pair<std::string, mesh::ColourPair>> membranes{
          {"outside-cell", {col0, col1}}};
      mesh::Contours c(img, compartments, membranes);
      REQUIRE(c.getBoundaries().size() == 2);
      REQUIRE(c.getFixedPoints().empty());
      const auto &b0 = c.getBoundaries()[0];
      REQUIRE(b0.isValid());
      REQUIRE(b0.isLoop());
      REQUIRE(b0.isMembrane());
      const auto &b1 = c.getBoundaries()[1];
      REQUIRE(b1.isValid());
      REQUIRE(b1.isLoop());
      REQUIRE(!b1.isMembrane());
    }
  }
  GIVEN("single cell with single boundary touching edge of image") {
    QStringList filenames{"gt1.png", "gt5.png", "gt9.png", "gt16.png"};
    for (const auto &filename : filenames) {
      CAPTURE(filename.toStdString());
      QImage img(QString(":/test/geometry/%1").arg(filename));

      // single compartment: black (outer)
      // single outer loop boundary
      mesh::Contours cBlack(img, {qRgb(0, 0, 0)}, {});
      REQUIRE(cBlack.getBoundaries().size() == 1);
      REQUIRE(cBlack.getFixedPoints().empty());
      for (const auto &b : cBlack.getBoundaries()) {
        REQUIRE(b.isValid());
        REQUIRE(b.isLoop());
        REQUIRE(!b.isMembrane());
      }

      // single compartment: white (inner)
      // single outer loop boundary
      mesh::Contours cWhite(img, {qRgb(255, 255, 255)}, {});
      REQUIRE(cWhite.getBoundaries().size() == 1);
      REQUIRE(cWhite.getFixedPoints().empty());
      const auto &bWhite = cWhite.getBoundaries()[0];
      REQUIRE(bWhite.isValid());
      REQUIRE(bWhite.isLoop());
      REQUIRE(!bWhite.isMembrane());

      // two compartments: black and white
      // three non-loop boundary lines, of which one membrane
      QRgb col0 = qRgb(0, 0, 0);
      QRgb col1 = qRgb(255, 255, 255);
      std::vector<QRgb> compartments{{col0, col1}};
      std::vector<std::pair<std::string, mesh::ColourPair>> membranes{
          {"outside-cell", {col0, col1}}};
      mesh::Contours c(img, compartments, membranes);
      REQUIRE(c.getBoundaries().size() == 3);
      REQUIRE(c.getFixedPoints().size() == 2);
      const auto &b0 = c.getBoundaries()[0];
      REQUIRE(b0.isValid());
      REQUIRE(!b0.isLoop());
      REQUIRE(!b0.isMembrane());
      const auto &b1 = c.getBoundaries()[1];
      REQUIRE(b1.isValid());
      REQUIRE(!b1.isLoop());
      REQUIRE(b1.isMembrane());
      const auto &b2 = c.getBoundaries()[2];
      REQUIRE(b2.isValid());
      REQUIRE(!b2.isLoop());
      REQUIRE(!b2.isMembrane());
    }
  }
  GIVEN("two cells, one touching edge of image") {
    QStringList filenames{"gt4.png", "gt7.png", "gt10.png"};
    for (const auto &filename : filenames) {
      CAPTURE(filename.toStdString());
      QImage img(QString(":/test/geometry/%1").arg(filename));

      // single compartment: black (outer)
      // pair of loop boundaries: single compartment with a hole in it
      mesh::Contours cBlack(img, {qRgb(0, 0, 0)}, {});
      REQUIRE(cBlack.getBoundaries().size() == 2);
      REQUIRE(cBlack.getFixedPoints().empty());
      for (const auto &b : cBlack.getBoundaries()) {
        REQUIRE(b.isValid());
        REQUIRE(b.isLoop());
        REQUIRE(!b.isMembrane());
      }

      // two independent compartments: white (inner)
      // 2 outer loop boundaries
      mesh::Contours cWhite(img, {qRgb(255, 255, 255)}, {});
      REQUIRE(cWhite.getBoundaries().size() == 2);
      REQUIRE(cWhite.getFixedPoints().empty());
      for (const auto &b : cWhite.getBoundaries()) {
        REQUIRE(b.isValid());
        REQUIRE(b.isLoop());
        REQUIRE(!b.isMembrane());
      }

      // 3 compartments: black and white
      // 3 non-loop boundary lines, of which 1 membrane + 1 loop membrane
      QRgb col0 = qRgb(0, 0, 0);
      QRgb col1 = qRgb(255, 255, 255);
      std::vector<QRgb> compartments{{col0, col1}};
      std::vector<std::pair<std::string, mesh::ColourPair>> membranes{
          {"outside-cell", {col0, col1}}};
      mesh::Contours c(img, compartments, membranes);
      REQUIRE(c.getBoundaries().size() == 4);
      REQUIRE(c.getFixedPoints().size() == 2);
      int nValid{0};
      int nLoops{0};
      int nMembranes{0};
      for (const auto &b : c.getBoundaries()) {
        nValid += static_cast<int>(b.isValid());
        nLoops += static_cast<int>(b.isLoop());
        nMembranes += static_cast<int>(b.isMembrane());
      }
      REQUIRE(nValid == 4);
      REQUIRE(nLoops == 1);
      REQUIRE(nMembranes == 2);
    }
  }
  GIVEN("two cells, both touching edge of image") {
    QStringList filenames{"gt6.png"};
    for (const auto &filename : filenames) {
      CAPTURE(filename.toStdString());
      QImage img(QString(":/test/geometry/%1").arg(filename));

      // single compartment: black (outer)
      // single loop boundary
      mesh::Contours cBlack(img, {qRgb(0, 0, 0)}, {});
      REQUIRE(cBlack.getBoundaries().size() == 1);
      REQUIRE(cBlack.getFixedPoints().empty());
      for (const auto &b : cBlack.getBoundaries()) {
        REQUIRE(b.isValid());
        REQUIRE(b.isLoop());
        REQUIRE(!b.isMembrane());
      }

      // two independent compartments: white (inner)
      // 2 outer loop boundaries
      mesh::Contours cWhite(img, {qRgb(255, 255, 255)}, {});
      REQUIRE(cWhite.getBoundaries().size() == 2);
      REQUIRE(cWhite.getFixedPoints().empty());
      for (const auto &b : cWhite.getBoundaries()) {
        REQUIRE(b.isValid());
        REQUIRE(b.isLoop());
        REQUIRE(!b.isMembrane());
      }

      // 3 compartments: black and white
      // many non-loop boundary lines, of which 2 membranes
      QRgb col0 = qRgb(0, 0, 0);
      QRgb col1 = qRgb(255, 255, 255);
      std::vector<QRgb> compartments{{col0, col1}};
      std::vector<std::pair<std::string, mesh::ColourPair>> membranes{
          {"outside-cell", {col0, col1}}};
      mesh::Contours c(img, compartments, membranes);
      REQUIRE(c.getBoundaries().size() >= 6);
      REQUIRE(c.getFixedPoints().size() >= 4);
      int nValid{0};
      int nLoops{0};
      int nMembranes{0};
      for (const auto &b : c.getBoundaries()) {
        nValid += static_cast<int>(b.isValid());
        nLoops += static_cast<int>(b.isLoop());
        nMembranes += static_cast<int>(b.isMembrane());
      }
      REQUIRE(nValid >= 6);
      REQUIRE(nLoops == 0);
      REQUIRE(nMembranes == 2);
    }
  }
  GIVEN("three cells with some boundaries touching edge of image") {
    QStringList filenames{"gt15.png"};
    for (const auto &filename : filenames) {
      CAPTURE(filename.toStdString());
      QImage img(QString(":/test/geometry/%1").arg(filename));

      // single compartment: black (outer)
      // single outer loop boundary
      mesh::Contours cBlack(img, {qRgb(0, 0, 0)}, {});
      REQUIRE(cBlack.getBoundaries().size() == 1);
      REQUIRE(cBlack.getFixedPoints().empty());
      for (const auto &b : cBlack.getBoundaries()) {
        REQUIRE(b.isValid());
        REQUIRE(b.isLoop());
        REQUIRE(!b.isMembrane());
      }

      // three independent compartments: white (inner)
      // 3 outer loop boundaries
      mesh::Contours cWhite(img, {qRgb(255, 255, 255)}, {});
      REQUIRE(cWhite.getBoundaries().size() == 3);
      REQUIRE(cWhite.getFixedPoints().empty());
      for (const auto &b : cWhite.getBoundaries()) {
        REQUIRE(b.isValid());
        REQUIRE(b.isLoop());
        REQUIRE(!b.isMembrane());
      }

      // 4 compartments: black and white
      // 9 non-loop boundary lines, of which 3 membranes
      QRgb col0 = qRgb(0, 0, 0);
      QRgb col1 = qRgb(255, 255, 255);
      std::vector<QRgb> compartments{{col0, col1}};
      std::vector<std::pair<std::string, mesh::ColourPair>> membranes{
          {"outside-cell", {col0, col1}}};
      mesh::Contours c(img, compartments, membranes);
      REQUIRE(c.getBoundaries().size() == 9);
      REQUIRE(c.getFixedPoints().size() == 6);
      int nValid{0};
      int nLoops{0};
      int nMembranes{0};
      for (const auto &b : c.getBoundaries()) {
        nValid += static_cast<int>(b.isValid());
        nLoops += static_cast<int>(b.isLoop());
        nMembranes += static_cast<int>(b.isMembrane());
      }
      REQUIRE(nValid == 9);
      REQUIRE(nLoops == 0);
      REQUIRE(nMembranes == 3);
    }
  }
  GIVEN("difficult images: at least don't crash") {
    QStringList filenames{"gt2.png", "gt8.png", "gt14.png",
                          "gt17.png"};
    for (const auto &filename : filenames) {
      CAPTURE(filename.toStdString());
      QImage img(QString(":/test/geometry/%1").arg(filename));

      // single compartment: black (outer)
      mesh::Contours cBlack(img, {qRgb(0, 0, 0)}, {});
      REQUIRE_NOTHROW(cBlack.getBoundaries());

      // single compartment: white (inner)
      mesh::Contours cWhite(img, {qRgb(255, 255, 255)}, {});
      REQUIRE_NOTHROW(cWhite.getBoundaries());

      // two compartments: black and white
      QRgb col0 = qRgb(0, 0, 0);
      QRgb col1 = qRgb(255, 255, 255);
      std::vector<QRgb> compartments{{col0, col1}};
      std::vector<std::pair<std::string, mesh::ColourPair>> membranes{
          {"outside-cell", {col0, col1}}};
      mesh::Contours c(img, compartments, membranes);
      REQUIRE_NOTHROW(c.getBoundaries());
    }
  }
}

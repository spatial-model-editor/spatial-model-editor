#include <QImage>
#include <QPoint>

#include "catch_wrapper.hpp"
#include "mesh.hpp"
#include "utils.hpp"

// note: only valid for vectors of unique values
static bool isCyclicPermutation(const std::vector<QPoint>& v1,
                                const std::vector<QPoint>& v2) {
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
  if (v1[(i0 + 1) % v1.size()] == v2[1]) {
    // match next element going forwards
    for (std::size_t i = 0; i < v1.size(); ++i) {
      // check the rest
      if (v1[(i0 + i) % v1.size()] != v2[i]) {
        return false;
      }
    }
  } else if (v1[(i0 + v1.size() - 1) % v1.size()] == v2[1]) {
    // match next element going backwards
    for (std::size_t i = 0; i < v1.size(); ++i) {
      // check the rest
      if (v1[(i0 + v1.size() - i) % v1.size()] != v2[i]) {
        return false;
      }
    }
  } else {
    // neither forwards or backwards match
    return false;
  }
  return true;
}

SCENARIO("Mesh", "[core][mesh][boundary]") {
  GIVEN("empty image") {
    QImage img(24, 32, QImage::Format_RGB32);
    QRgb col = QColor(0, 0, 0).rgb();
    img.fill(col);
    // image boundary pixels
    std::vector<QPoint> imgBoundary;
    imgBoundary.emplace_back(0, 0);
    imgBoundary.emplace_back(0, img.height() - 1);
    imgBoundary.emplace_back(img.width() - 1, img.height() - 1);
    imgBoundary.emplace_back(img.width() - 1, 0);

    mesh::Mesh mesh;
    mesh = mesh::Mesh(img, {QPointF(8, 8)}, {}, {999}, {}, {}, 1.0,
                      QPointF(0, 0), std::vector<QRgb>{col});

    // check boundaries
    const auto& boundaries = mesh.getBoundaries();
    CAPTURE(boundaries[0].points);
    REQUIRE(boundaries.size() == 1);
    REQUIRE(boundaries[0].isLoop);
    REQUIRE(isCyclicPermutation(boundaries[0].points, imgBoundary));

    // check output vertices
    const auto& vertices = mesh.getVertices();
    REQUIRE(vertices.size() == 2 * 4);

    // check triangles
    const auto& triangles = mesh.getTriangles();
    REQUIRE(triangles.size() == 1);
    REQUIRE(triangles[0].size() == 2);
    REQUIRE(triangles[0][0] ==
            std::array<QPointF, 3>{
                {QPointF(0, 31), QPointF(0, 0), QPointF(23, 0)}});
    REQUIRE(triangles[0][1] ==
            std::array<QPointF, 3>{
                {QPointF(23, 0), QPointF(23, 31), QPointF(0, 31)}});

    // check gmsh output
    QStringList msh = mesh.getGMSH().split("\n");
    REQUIRE(msh[0] == "$MeshFormat");
    REQUIRE(msh[1] == "2.2 0 8");
    REQUIRE(msh[2] == "$EndMeshFormat");
    REQUIRE(msh[3] == "$Nodes");
    REQUIRE(msh[4] == "4");
    REQUIRE(msh[5] == "1 23 0 0");
    REQUIRE(msh[6] == "2 0 0 0");
    REQUIRE(msh[7] == "3 23 31 0");
    REQUIRE(msh[8] == "4 0 31 0");
    REQUIRE(msh[9] == "$EndNodes");
    REQUIRE(msh[10] == "$Elements");
    REQUIRE(msh[11] == "2");
    REQUIRE(msh[12] == "1 2 2 1 1 4 2 1");
    REQUIRE(msh[13] == "2 2 2 1 1 1 3 4");
    REQUIRE(msh[14] == "$EndElements");

    // check image output
    auto [boundaryImage, maskImage] =
        mesh.getBoundariesImages(QSize(100, 100), 0);
    auto col0 = utils::indexedColours()[0].rgba();
    boundaryImage.save("xx.png");
    REQUIRE(boundaryImage.width() == 75);
    REQUIRE(boundaryImage.height() == 100);
    REQUIRE(boundaryImage.pixel(3, 3) == col0);
    REQUIRE(boundaryImage.pixel(34, 2) == col0);
    REQUIRE(boundaryImage.pixel(69, 4) == col0);
    REQUIRE(boundaryImage.pixel(70, 51) == col0);
    REQUIRE(boundaryImage.pixel(67, 95) == col0);
    REQUIRE(boundaryImage.pixel(30, 99) == col0);
    REQUIRE(boundaryImage.pixel(2, 94) == col0);
    REQUIRE(boundaryImage.pixel(9, 11) == 0);
    REQUIRE(boundaryImage.pixel(54, 19) == 0);
    REQUIRE(boundaryImage.pixel(53, 78) == 0);
    REQUIRE(boundaryImage.pixel(9, 81) == 0);

    auto [meshImage, meshMaskImage] = mesh.getMeshImages(QSize(100, 100), 0);
    REQUIRE(meshImage.width() == 75);
    REQUIRE(meshImage.height() == 100);

    // check rescaling of image
    auto [boundaryImage2, maskImage2] =
        mesh.getBoundariesImages(QSize(30, 120), 0);
    REQUIRE(boundaryImage2.width() == 30);
    REQUIRE(boundaryImage2.height() == 40);

    WHEN("max boundary points increased") {
      THEN("boundary unchanged") {
        REQUIRE(mesh.getBoundaryMaxPoints(0) == 12);
        auto oldBoundaryPoints = mesh.getBoundaries()[0].points;
        mesh.setBoundaryMaxPoints(0, 99);
        REQUIRE(mesh.getBoundaryMaxPoints(0) == 99);
        REQUIRE(mesh.getBoundaries()[0].points == oldBoundaryPoints);
      }
    }
    WHEN("max triangle area decreased") {
      THEN("more vertices & triangles") {
        REQUIRE(mesh.getCompartmentMaxTriangleArea(0) == 999);

        mesh.setCompartmentMaxTriangleArea(0, 60);
        REQUIRE(mesh.getCompartmentMaxTriangleArea(0) == 60);
        REQUIRE(mesh.getVertices().size() == 2 * 14);
        REQUIRE(mesh.getTriangles()[0].size() == 18);

        mesh.setCompartmentMaxTriangleArea(0, 30);
        REQUIRE(mesh.getCompartmentMaxTriangleArea(0) == 30);
        REQUIRE(mesh.getVertices().size() == 2 * 26);
        REQUIRE(mesh.getTriangles()[0].size() == 37);

        mesh.setCompartmentMaxTriangleArea(0, 12);
        REQUIRE(mesh.getCompartmentMaxTriangleArea(0) == 12);
        REQUIRE(mesh.getVertices().size() == 2 * 54);
        REQUIRE(mesh.getTriangles()[0].size() == 88);
      }
    }
  }
  GIVEN("single convex compartment") {
    QImage img(24, 32, QImage::Format_RGB32);
    QRgb bgcol = QColor(0, 0, 0).rgb();
    img.fill(bgcol);
    // image boundary pixels
    std::vector<QPoint> imgBoundary;
    imgBoundary.emplace_back(0, 0);
    imgBoundary.emplace_back(0, img.height() - 1);
    imgBoundary.emplace_back(img.width() - 1, img.height() - 1);
    imgBoundary.emplace_back(img.width() - 1, 0);

    // compartment boundary pixels
    std::vector<QPoint> correctBoundary;
    correctBoundary.emplace_back(5, 5);
    correctBoundary.emplace_back(6, 5);
    correctBoundary.emplace_back(7, 6);
    correctBoundary.emplace_back(6, 7);
    correctBoundary.emplace_back(5, 7);
    correctBoundary.emplace_back(4, 6);
    QRgb col = QColor(123, 123, 123).rgb();
    for (const auto& p : correctBoundary) {
      img.setPixel(p, col);
    }
    // fill in internal compartment pixels
    img.setPixel(5, 6, col);
    img.setPixel(6, 6, col);
    // flip y-axis to match (0,0) == bottom-left of meshing output
    img = img.mirrored(false, true);

    mesh::Mesh mesh(img, {QPointF(6, 6)}, {}, {999}, {}, {}, 1.0, QPointF(0, 0),
                    std::vector<QRgb>{bgcol, col});

    // check boundaries
    const auto& boundaries = mesh.getBoundaries();
    CAPTURE(boundaries[0].points);
    CAPTURE(boundaries[1].points);
    REQUIRE(boundaries.size() == 2);
    REQUIRE(boundaries[0].isLoop);
    REQUIRE(boundaries[1].isLoop);
    REQUIRE(isCyclicPermutation(boundaries[0].points, imgBoundary));
    REQUIRE(isCyclicPermutation(boundaries[1].points, correctBoundary));

    auto [boundaryImage, maskImage] =
        mesh.getBoundariesImages(QSize(100, 100), 0);
    boundaryImage.save("tmp0.png");
    REQUIRE(boundaryImage.width() == 75);
    REQUIRE(boundaryImage.height() == 100);
    auto col0 = utils::indexedColours()[0].rgba();
    auto col1 = utils::indexedColours()[1].rgba();
    REQUIRE(boundaryImage.pixel(1, 3) == col0);
    REQUIRE(boundaryImage.pixel(3, 6) == col0);
    REQUIRE(boundaryImage.pixel(70, 62) == col0);
    REQUIRE(boundaryImage.pixel(70, 2) == col0);
    REQUIRE(boundaryImage.pixel(5, 78) == 0);
    REQUIRE(boundaryImage.pixel(17, 80) != col1);
    REQUIRE(boundaryImage.pixel(26, 80) == 0);
    auto [boundaryImage2, maskImage2] =
        mesh.getBoundariesImages(QSize(100, 100), 1);
    boundaryImage2.save("tmp1.png");
    REQUIRE(boundaryImage2.pixel(1, 3) == col0);
    REQUIRE(boundaryImage2.pixel(3, 6) == 0);
    REQUIRE(boundaryImage2.pixel(72, 62) == col0);
    REQUIRE(boundaryImage2.pixel(72, 2) == col0);
    REQUIRE(boundaryImage2.pixel(8, 78) == col1);
    REQUIRE(boundaryImage2.pixel(16, 79) == col1);
    REQUIRE(boundaryImage2.pixel(26, 80) == col1);
  }
  GIVEN("single convex compartment, degenerate points") {
    QImage img(24, 32, QImage::Format_RGB32);
    QRgb bgcol = QColor(0, 0, 0).rgb();
    img.fill(bgcol);
    // image boundary pixels
    std::vector<QPoint> imgBoundary;
    imgBoundary.emplace_back(0, 0);
    imgBoundary.emplace_back(0, img.height() - 1);
    imgBoundary.emplace_back(img.width() - 1, img.height() - 1);
    imgBoundary.emplace_back(img.width() - 1, 0);

    // compartment boundary pixels
    std::vector<QPoint> correctBoundary;
    correctBoundary.emplace_back(5, 5);
    correctBoundary.emplace_back(8, 5);
    correctBoundary.emplace_back(9, 6);
    correctBoundary.emplace_back(7, 8);
    correctBoundary.emplace_back(6, 8);
    correctBoundary.emplace_back(4, 6);
    QRgb col = QColor(123, 123, 123).rgb();
    for (const auto& p : correctBoundary) {
      img.setPixel(p, col);
    }
    // fill in straight boundary pixels
    img.setPixel(6, 5, col);
    img.setPixel(7, 5, col);
    img.setPixel(8, 7, col);
    img.setPixel(5, 7, col);
    // fill in internal compartment pixels
    img.setPixel(5, 6, col);
    img.setPixel(6, 6, col);
    img.setPixel(7, 6, col);
    img.setPixel(8, 6, col);
    img.setPixel(6, 7, col);
    img.setPixel(7, 7, col);

    // flip y-axis to match (0,0) == bottom-left of meshing output
    img = img.mirrored(false, true);

    mesh::Mesh mesh(img, {QPointF(7, 6)}, {}, {999}, {}, {}, 1.0, QPointF(0, 0),
                    std::vector<QRgb>{bgcol, col});

    const auto& boundaries = mesh.getBoundaries();
    CAPTURE(boundaries[0].points);
    CAPTURE(boundaries[1].points);
    REQUIRE(boundaries[0].isLoop);
    REQUIRE(boundaries[1].isLoop);
    REQUIRE(isCyclicPermutation(boundaries[0].points, imgBoundary));
    REQUIRE(isCyclicPermutation(boundaries[1].points, correctBoundary));
  }
  GIVEN("single concave compartment") {
    QImage img(24, 32, QImage::Format_RGB32);
    QRgb bgcol = QColor(0, 0, 0).rgb();
    img.fill(bgcol);
    // image boundary pixels
    std::vector<QPoint> imgBoundary;
    imgBoundary.emplace_back(0, 0);
    imgBoundary.emplace_back(0, img.height() - 1);
    imgBoundary.emplace_back(img.width() - 1, img.height() - 1);
    imgBoundary.emplace_back(img.width() - 1, 0);

    // compartment boundary pixels
    std::vector<QPoint> correctBoundary;
    correctBoundary.emplace_back(12, 10);
    correctBoundary.emplace_back(15, 10);
    correctBoundary.emplace_back(15, 12);
    correctBoundary.emplace_back(14, 12);
    correctBoundary.emplace_back(13, 13);
    correctBoundary.emplace_back(15, 15);
    correctBoundary.emplace_back(15, 16);
    correctBoundary.emplace_back(12, 16);
    correctBoundary.emplace_back(10, 14);
    correctBoundary.emplace_back(10, 13);
    correctBoundary.emplace_back(12, 11);
    QRgb col = QColor(123, 123, 123).rgb();
    for (const auto& p : correctBoundary) {
      img.setPixel(p, col);
    }
    // fill in degenerate boundary points
    img.setPixel(13, 10, col);
    img.setPixel(14, 10, col);
    img.setPixel(15, 11, col);
    img.setPixel(14, 14, col);
    img.setPixel(14, 16, col);
    img.setPixel(13, 16, col);
    img.setPixel(11, 15, col);
    img.setPixel(11, 12, col);
    // fill in internal compartment pixels
    img.setPixel(13, 11, col);
    img.setPixel(14, 11, col);
    img.setPixel(12, 12, col);
    img.setPixel(13, 12, col);
    img.setPixel(13, 11, col);
    img.setPixel(11, 13, col);
    img.setPixel(12, 13, col);
    img.setPixel(11, 14, col);
    img.setPixel(12, 14, col);
    img.setPixel(12, 15, col);
    img.setPixel(13, 14, col);
    img.setPixel(13, 15, col);
    img.setPixel(14, 15, col);

    // flip y-axis to match (0,0) == bottom-left of meshing output
    img = img.mirrored(false, true);

    mesh::Mesh mesh(img, {QPointF(12, 13)}, {}, {999}, {}, {}, 1.0,
                    QPointF(0, 0), std::vector<QRgb>{bgcol, col});

    const auto& boundaries = mesh.getBoundaries();
    CAPTURE(boundaries[0].points);
    CAPTURE(boundaries[1].points);
    REQUIRE(boundaries[0].isLoop);
    REQUIRE(boundaries[1].isLoop);
    REQUIRE(isCyclicPermutation(boundaries[0].points, imgBoundary));
    REQUIRE(isCyclicPermutation(boundaries[1].points, correctBoundary));
  }
  GIVEN("two touching compartments, two fixed point pixels") {
    QImage img(":/geometry/two-blobs-100x100.png");
    QRgb colblack = QColor(0, 2, 0).rgb();
    QRgb colgrey = QColor(132, 134, 131).rgb();
    QRgb colwhite = QColor(250, 252, 249).rgb();
    std::vector<QPointF> interiorPoints = {QPointF(12, 12), QPointF(33, 66),
                                           QPointF(66, 33)};
    mesh::Mesh mesh(img, interiorPoints, {}, {999, 999, 999}, {}, {}, 1.0,
                    QPointF(0, 0),
                    std::vector<QRgb>{colblack, colgrey, colwhite});

    const auto& boundaries = mesh.getBoundaries();
    REQUIRE(boundaries.size() == 4);
    REQUIRE(!boundaries[0].isLoop);
    REQUIRE(!boundaries[1].isLoop);
    REQUIRE(!boundaries[2].isLoop);
    REQUIRE(boundaries[3].isLoop);
  }
  GIVEN("concave cell nucleus, one membrane") {
    QImage img(":/geometry/concave-cell-nucleus-100x100.png");
    QRgb c1 = QColor(144, 97, 193).rgb();
    QRgb c2 = QColor(197, 133, 96).rgb();
    mesh::Mesh mesh(img, {{QPointF(28, 27), QPointF(40, 52)}}, {{5, 12, 16}},
                    {{15, 19}}, {{{"membrane", {c1, c2}}}}, {}, 1.0,
                    QPointF(0, 0), std::vector<QRgb>{c1, c2, 0xff000200});

    const auto& boundaries = mesh.getBoundaries();
    REQUIRE(boundaries.size() == 3);
    REQUIRE(boundaries[0].getMaxPoints() == 5);
    REQUIRE(boundaries[0].isLoop == true);
    REQUIRE(boundaries[0].isMembrane == false);
    REQUIRE(boundaries[1].getMaxPoints() == 12);
    REQUIRE(boundaries[1].isLoop == true);
    REQUIRE(boundaries[1].isMembrane == false);
    REQUIRE(boundaries[2].getMaxPoints() == 16);
    REQUIRE(boundaries[2].isLoop == true);
    REQUIRE(boundaries[2].isMembrane == true);

    // change membrane width
    for (double w : {0.2, 0.6, 1.0, 2.0, 3.0}) {
      mesh.setBoundaryWidth(2, w);
      REQUIRE(mesh.getBoundaryWidth(2) == dbl_approx(w));
      auto diff = mesh.getBoundaries()[2].points[5] -
                  mesh.getBoundaries()[2].outerPoints[5];
      double dot = QPointF::dotProduct(diff, diff);
      REQUIRE(sqrt(dot) == Approx(w));
    }

    auto [boundaryImage, maskImage] =
        mesh.getBoundariesImages(QSize(200, 200), 0);
    REQUIRE(boundaryImage.size() == QSize(200, 200));
  }
}

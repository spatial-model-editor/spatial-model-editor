#include <QImage>
#include <QPoint>

#include "catch_wrapper.hpp"
#include "mesh.hpp"
#include "utils.hpp"

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
    REQUIRE(mesh.getNumBoundaries() == 1);

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
    REQUIRE(msh[5] == "1 0 0 0");
    REQUIRE(msh[6] == "2 23 0 0");
    REQUIRE(msh[7] == "3 23 31 0");
    REQUIRE(msh[8] == "4 0 31 0");
    REQUIRE(msh[9] == "$EndNodes");
    REQUIRE(msh[10] == "$Elements");
    REQUIRE(msh[11] == "2");
    REQUIRE(msh[12] == "1 2 2 1 1 4 1 2");
    REQUIRE(msh[13] == "2 2 2 1 1 2 3 4");
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
        REQUIRE(mesh.getBoundaryMaxPoints(0) == 4);
        auto nVertices = mesh.getVertices().size();
        mesh.setBoundaryMaxPoints(0, 99);
        REQUIRE(mesh.getBoundaryMaxPoints(0) == 99);
        REQUIRE(mesh.getVertices().size() == nVertices);
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

    // check boundaries image
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
    REQUIRE(boundaryImage.pixel(17, 80) == 0);
    REQUIRE(boundaryImage.pixel(26, 80) == col1);
    auto [boundaryImage2, maskImage2] =
        mesh.getBoundariesImages(QSize(100, 100), 1);
    boundaryImage2.save("tmp1.png");
    REQUIRE(boundaryImage2.pixel(1, 3) == col0);
    REQUIRE(boundaryImage2.pixel(3, 6) == 0);
    REQUIRE(boundaryImage2.pixel(15, 81) == col1);
    REQUIRE(boundaryImage2.pixel(26, 80) == col1);
  }
}

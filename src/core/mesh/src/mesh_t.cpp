#include "catch_wrapper.hpp"
#include "mesh.hpp"
#include "utils.hpp"
#include <QImage>
#include <QPoint>

using namespace sme;

SCENARIO("Mesh", "[core/mesh/mesh][core/mesh][core][mesh]") {
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

    mesh::Mesh mesh(img, {}, {999}, 1.0, QPointF(0, 0), std::vector<QRgb>{col});

    // check boundaries
    REQUIRE(mesh.getNumBoundaries() == 1);

    // check output vertices
    const auto &vertices = mesh.getVerticesAsFlatArray();
    REQUIRE(vertices.size() == 2 * 4);

    // check triangles
    const auto &triangles = mesh.getTriangleIndices();
    REQUIRE(triangles.size() == 1);
    REQUIRE(triangles[0].size() == 2);

    // check gmsh output
    QStringList msh = mesh.getGMSH().split("\n");
    REQUIRE(msh[0] == "$MeshFormat");
    REQUIRE(msh[1] == "2.2 0 8");
    REQUIRE(msh[2] == "$EndMeshFormat");
    REQUIRE(msh[3] == "$Nodes");
    REQUIRE(msh[4] == "4");
    // 4x lines, 1 for each node
    REQUIRE(msh[9] == "$EndNodes");
    REQUIRE(msh[10] == "$Elements");
    REQUIRE(msh[11] == "2");
    // 2x lines, 1 for each element
    REQUIRE(msh[14] == "$EndElements");

    // check image output
    auto [boundaryImage, maskImage] =
        mesh.getBoundariesImages(QSize(100, 100), 0);
    auto col0 = common::indexedColours()[0].rgba();
    boundaryImage.save("xx.png");
    REQUIRE(boundaryImage.width() == 77);
    REQUIRE(boundaryImage.height() == 100);
    REQUIRE(boundaryImage.pixel(1, 1) == col0);
    REQUIRE(boundaryImage.pixel(8, 8) == col0);
    REQUIRE(boundaryImage.pixel(73, 4) == col0);
    REQUIRE(boundaryImage.pixel(73, 51) == col0);
    REQUIRE(boundaryImage.pixel(72, 93) == col0);
    REQUIRE(boundaryImage.pixel(30, 94) == col0);
    REQUIRE(boundaryImage.pixel(1, 95) == col0);
    REQUIRE(boundaryImage.pixel(9, 11) == 0);
    REQUIRE(boundaryImage.pixel(54, 19) == 0);
    REQUIRE(boundaryImage.pixel(53, 78) == 0);
    REQUIRE(boundaryImage.pixel(9, 81) == 0);

    auto [meshImage, meshMaskImage] = mesh.getMeshImages(QSize(100, 100), 0);
    REQUIRE(meshImage.width() == 77);
    REQUIRE(meshImage.height() == 100);

    // check rescaling of image
    auto [boundaryImage2, maskImage2] =
        mesh.getBoundariesImages(QSize(30, 120), 0);
    REQUIRE(boundaryImage2.width() == 30);
    REQUIRE(boundaryImage2.height() == 36);

    WHEN("max boundary points increased") {
      THEN("boundary unchanged") {
        REQUIRE(mesh.getBoundaryMaxPoints(0) == 4);
        auto nVertices = mesh.getVerticesAsFlatArray().size();
        mesh.setBoundaryMaxPoints(0, 99);
        REQUIRE(mesh.getBoundaryMaxPoints(0) == 99);
        REQUIRE(mesh.getVerticesAsFlatArray().size() == nVertices);
      }
    }
    WHEN("max triangle area decreased") {
      THEN("more vertices & triangles") {
        REQUIRE(mesh.getCompartmentMaxTriangleArea(0) == 999);
        std::size_t oldNTriangles{mesh.getTriangleIndices()[0].size()};
        std::size_t oldNVertices{mesh.getVerticesAsFlatArray().size() / 2};

        mesh.setCompartmentMaxTriangleArea(0, 60);
        std::size_t newNTriangles{mesh.getTriangleIndices()[0].size()};
        std::size_t newNVertices{mesh.getVerticesAsFlatArray().size() / 2};
        REQUIRE(mesh.getCompartmentMaxTriangleArea(0) == 60);
        REQUIRE(newNTriangles > oldNTriangles);
        REQUIRE(newNVertices > oldNVertices);

        std::swap(oldNTriangles, newNTriangles);
        std::swap(oldNVertices, newNVertices);
        mesh.setCompartmentMaxTriangleArea(0, 30);
        newNTriangles = mesh.getTriangleIndices()[0].size();
        newNVertices = mesh.getVerticesAsFlatArray().size() / 2;
        REQUIRE(mesh.getCompartmentMaxTriangleArea(0) == 30);
        REQUIRE(newNTriangles > oldNTriangles);
        REQUIRE(newNVertices > oldNVertices);

        std::swap(oldNTriangles, newNTriangles);
        std::swap(oldNVertices, newNVertices);
        mesh.setCompartmentMaxTriangleArea(0, 12);
        newNTriangles = mesh.getTriangleIndices()[0].size();
        newNVertices = mesh.getVerticesAsFlatArray().size() / 2;
        REQUIRE(mesh.getCompartmentMaxTriangleArea(0) == 12);
        REQUIRE(newNTriangles > oldNTriangles);
        REQUIRE(newNVertices > oldNVertices);

        std::swap(oldNTriangles, newNTriangles);
        std::swap(oldNVertices, newNVertices);
        mesh.setCompartmentMaxTriangleArea(0, 2);
        newNTriangles = mesh.getTriangleIndices()[0].size();
        newNVertices = mesh.getVerticesAsFlatArray().size() / 2;
        REQUIRE(mesh.getCompartmentMaxTriangleArea(0) == 2);
        REQUIRE(newNTriangles > oldNTriangles);
        REQUIRE(newNVertices > oldNVertices);
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
    for (const auto &p : correctBoundary) {
      img.setPixel(p, col);
    }
    // fill in internal compartment pixels
    img.setPixel(5, 6, col);
    img.setPixel(6, 6, col);
    // flip y-axis to match (0,0) == bottom-left of meshing output
    img = img.mirrored(false, true);

    mesh::Mesh mesh(img, {}, {999, 999}, 1.0, QPointF(0, 0),
                    std::vector<QRgb>{bgcol, col});

    // check boundaries image
    auto [boundaryImage, maskImage] =
        mesh.getBoundariesImages(QSize(100, 100), 0);
    boundaryImage.save("tmp0.png");
    REQUIRE(boundaryImage.width() == 77);
    REQUIRE(boundaryImage.height() == 100);
    auto col0 = common::indexedColours()[0].rgba();
    auto col1 = common::indexedColours()[1].rgba();
    REQUIRE(boundaryImage.pixel(1, 1) == col0);
    REQUIRE(boundaryImage.pixel(4, 5) == col0);
    REQUIRE(boundaryImage.pixel(72, 96) == col0);
    REQUIRE(boundaryImage.pixel(72, 1) == col0);
    REQUIRE(boundaryImage.pixel(22, 76) == 0);
    // anti-aliasing means pixel may not have exactly the expected color:
    REQUIRE(qRed(boundaryImage.pixel(25, 80)) ==
            Catch::Approx(qRed(col1)).margin(10));
    REQUIRE(qGreen(boundaryImage.pixel(25, 80)) ==
            Catch::Approx(qGreen(col1)).margin(10));
    REQUIRE(qBlue(boundaryImage.pixel(25, 80)) ==
            Catch::Approx(qBlue(col1)).margin(10));
    auto [boundaryImage2, maskImage2] =
        mesh.getBoundariesImages(QSize(100, 100), 1);
    boundaryImage2.save("tmp1.png");
    REQUIRE(boundaryImage2.pixel(4, 5) == col0);
    REQUIRE(boundaryImage2.pixel(8, 7) == 0);
    REQUIRE(boundaryImage2.pixel(26, 75) == col1);
    REQUIRE(boundaryImage2.pixel(20, 76) == col1);
  }
  GIVEN("interior point outside compartment") {
    // https://github.com/spatial-model-editor/spatial-model-editor/issues/585
    QImage img(3, 1, QImage::Format_RGB32);
    QRgb col = QColor(0, 0, 0).rgb();
    img.fill(col);
    std::size_t maxTriangleArea{999};
    mesh::Mesh mesh(img, {}, {maxTriangleArea}, 1.0, QPointF(0, 0),
                    std::vector<QRgb>{col});
    // use 3 point boundary around 3x1 pixel rectangle:
    // interior point is outside this boundary
    mesh.setBoundaryMaxPoints(0, 3);
    REQUIRE(mesh.isValid() == false);
    REQUIRE(mesh.getErrorMessage() ==
            "Triangle is outside of the boundary lines");
    // setting max boundary points explicitly to 4 results in a valid mesh
    mesh.setBoundaryMaxPoints(0, 4);
    // re-generate mesh
    mesh.setCompartmentMaxTriangleArea(0, maxTriangleArea);
    REQUIRE(mesh.isValid() == true);
    REQUIRE(mesh.getErrorMessage().empty());
    REQUIRE(mesh.getNumBoundaries() == 1);
    REQUIRE(mesh.getVerticesAsFlatArray().size() >= 4);
    REQUIRE(mesh.getTriangleIndices().size() == 1);
    REQUIRE(mesh.getTriangleIndices()[0].size() >= 2);
    // repeat to check validity & error message updated correctly
    mesh.setBoundaryMaxPoints(0, 2);
    // re-generate mesh
    mesh.setCompartmentMaxTriangleArea(0, maxTriangleArea);
    REQUIRE(mesh.isValid() == false);
    REQUIRE(mesh.isValid() == false);
    REQUIRE(mesh.getErrorMessage() ==
            "Triangle is outside of the boundary lines");
    mesh.setBoundaryMaxPoints(0, 4);
    // re-generate mesh
    mesh.setCompartmentMaxTriangleArea(0, maxTriangleArea);
    REQUIRE(mesh.isValid() == true);
    REQUIRE(mesh.getErrorMessage().empty());
  }
}

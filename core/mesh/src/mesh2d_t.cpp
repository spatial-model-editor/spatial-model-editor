#include "catch_wrapper.hpp"
#include "sme/mesh2d.hpp"
#include "sme/utils.hpp"
#include <QImage>
#include <QPoint>

using namespace sme;

TEST_CASE("Mesh", "[core/mesh/mesh][core/mesh][core][mesh]") {
  SECTION("empty image") {
    QImage img(24, 32, QImage::Format_RGB32);
    QRgb col = QColor(0, 0, 0).rgb();
    img.fill(col);
    // image boundary pixels
    std::vector<QPoint> imgBoundary;
    imgBoundary.emplace_back(0, 0);
    imgBoundary.emplace_back(0, img.height() - 1);
    imgBoundary.emplace_back(img.width() - 1, img.height() - 1);
    imgBoundary.emplace_back(img.width() - 1, 0);
    mesh::Mesh2d mesh(img, {}, {999}, {1.0, 1.0, 1.0}, {0, 0, 0},
                      std::vector<QRgb>{col});
    SECTION("check outputs") {
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
      auto col0 = common::indexedColors()[0].rgba();
      REQUIRE(boundaryImage[0].width() == 77);
      REQUIRE(boundaryImage[0].height() == 100);
      REQUIRE(boundaryImage[0].pixel(1, 1) == col0);
      REQUIRE(boundaryImage[0].pixel(8, 8) == col0);
      REQUIRE(boundaryImage[0].pixel(73, 4) == col0);
      REQUIRE(boundaryImage[0].pixel(73, 51) == col0);
      REQUIRE(boundaryImage[0].pixel(72, 93) == col0);
      REQUIRE(boundaryImage[0].pixel(30, 94) == col0);
      REQUIRE(boundaryImage[0].pixel(1, 95) == col0);
      REQUIRE(boundaryImage[0].pixel(9, 11) == 0);
      REQUIRE(boundaryImage[0].pixel(54, 19) == 0);
      REQUIRE(boundaryImage[0].pixel(53, 78) == 0);
      REQUIRE(boundaryImage[0].pixel(9, 81) == 0);

      auto [meshImage, meshMaskImage] = mesh.getMeshImages(QSize(100, 100), 0);
      REQUIRE(meshImage[0].width() == 77);
      REQUIRE(meshImage[0].height() == 100);

      // check rescaling of image
      auto [boundaryImage2, maskImage2] =
          mesh.getBoundariesImages(QSize(30, 120), 0);
      REQUIRE(boundaryImage2[0].width() == 30);
      REQUIRE(boundaryImage2[0].height() == 36);
    }
    SECTION("max boundary points increased") {
      REQUIRE(mesh.getBoundaryMaxPoints(0) == 4);
      auto nVertices = mesh.getVerticesAsFlatArray().size();
      mesh.setBoundaryMaxPoints(0, 99);
      REQUIRE(mesh.getBoundaryMaxPoints(0) == 99);
      REQUIRE(mesh.getVerticesAsFlatArray().size() == nVertices);
    }
    SECTION("max triangle area decreased") {
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
  SECTION("single convex compartment") {
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

    mesh::Mesh2d mesh(img, {}, {999, 999}, {1.0, 1.0, 1.0}, {0, 0, 0},
                      std::vector<QRgb>{bgcol, col});

    // check boundaries image
    auto [boundaryImage, maskImage] =
        mesh.getBoundariesImages(QSize(100, 100), 0);
    REQUIRE(boundaryImage[0].width() == 77);
    REQUIRE(boundaryImage[0].height() == 100);
    auto col0 = common::indexedColors()[0].rgba();
    auto col1 = common::indexedColors()[1].rgba();
    REQUIRE(boundaryImage[0].pixel(1, 1) == col0);
    REQUIRE(boundaryImage[0].pixel(4, 5) == col0);
    REQUIRE(boundaryImage[0].pixel(72, 96) == col0);
    REQUIRE(boundaryImage[0].pixel(72, 1) == col0);
    REQUIRE(boundaryImage[0].pixel(22, 76) == 0);
    // anti-aliasing means pixel may not have exactly the expected color:
    REQUIRE(qRed(boundaryImage[0].pixel(24, 73)) ==
            Catch::Approx(qRed(col1)).margin(10));
    REQUIRE(qGreen(boundaryImage[0].pixel(24, 73)) ==
            Catch::Approx(qGreen(col1)).margin(10));
    REQUIRE(qBlue(boundaryImage[0].pixel(24, 73)) ==
            Catch::Approx(qBlue(col1)).margin(10));
    auto [boundaryImage2, maskImage2] =
        mesh.getBoundariesImages(QSize(100, 100), 1);
    REQUIRE(boundaryImage2[0].pixel(4, 5) == col0);
    REQUIRE(boundaryImage2[0].pixel(8, 7) == 0);
    REQUIRE(boundaryImage2[0].pixel(26, 75) == col1);
    REQUIRE(boundaryImage2[0].pixel(20, 76) == col1);
  }
  SECTION("interior point outside compartment") {
    // https://github.com/spatial-model-editor/spatial-model-editor/issues/585
    QImage img(3, 1, QImage::Format_RGB32);
    QRgb col = QColor(0, 0, 0).rgb();
    img.fill(col);
    std::size_t maxTriangleArea{999};
    mesh::Mesh2d mesh(img, {}, {maxTriangleArea}, {1.0, 1.0, 1.0}, {0, 0, 0},
                      std::vector<QRgb>{col});
    REQUIRE(mesh.isValid() == true);
    // use 3 point boundary around 3x1 pixel rectangle:
    // interior point is outside this boundary
    mesh.setBoundaryMaxPoints(0, 3);
    // re-generate mesh
    mesh.constructMesh();
    REQUIRE(mesh.isValid() == false);
    REQUIRE(mesh.getErrorMessage() ==
            "Triangle is outside of the boundary lines");
    // setting max boundary points explicitly to 4 results in a valid mesh
    mesh.setBoundaryMaxPoints(0, 4);
    // re-generate mesh
    mesh.constructMesh();
    REQUIRE(mesh.isValid() == true);
    REQUIRE(mesh.getErrorMessage().empty());
    REQUIRE(mesh.getNumBoundaries() == 1);
    REQUIRE(mesh.getVerticesAsFlatArray().size() >= 4);
    REQUIRE(mesh.getTriangleIndices().size() == 1);
    REQUIRE(mesh.getTriangleIndices()[0].size() >= 2);
    // repeat to check validity & error message updated correctly
    mesh.setBoundaryMaxPoints(0, 2);
    // re-generate mesh
    mesh.constructMesh();
    REQUIRE(mesh.isValid() == false);
    REQUIRE(mesh.isValid() == false);
    REQUIRE(mesh.getErrorMessage() ==
            "Triangle is outside of the boundary lines");
    mesh.setBoundaryMaxPoints(0, 4);
    // re-generate mesh
    mesh.constructMesh();
    REQUIRE(mesh.isValid() == true);
    REQUIRE(mesh.getErrorMessage().empty());
  }
  SECTION("low resolution image -> small default triangle area") {
    QImage img(16, 12, QImage::Format_RGB32);
    QRgb col = QColor(0, 0, 0).rgb();
    img.fill(col);
    mesh::Mesh2d mesh(img, {}, {}, {1.0, 1.0, 1.0}, {0, 0, 0},
                      std::vector<QRgb>{col});
    REQUIRE(mesh.getCompartmentMaxTriangleArea(0) < 4);
  }
  SECTION("high resolution image -> large default triangle area") {
    QImage img(3840, 2160, QImage::Format_RGB32);
    QRgb col = QColor(32, 1, 88).rgb();
    img.fill(col);
    mesh::Mesh2d mesh(img, {}, {}, {1.0, 1.0, 1.0}, {0, 0, 0},
                      std::vector<QRgb>{col});
    REQUIRE(mesh.getCompartmentMaxTriangleArea(0) > 10000);
  }
}

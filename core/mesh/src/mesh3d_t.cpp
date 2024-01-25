#include "catch_wrapper.hpp"
#include "model_test_utils.hpp"
#include "sme/image_stack.hpp"
#include "sme/mesh3d.hpp"
#include "sme/tiff.hpp"
#include "sme/utils.hpp"
#include <QDir>
#include <QFile>
#include <QImage>
#include <QPoint>
#include <algorithm>

using namespace sme;
using Catch::Matchers::ContainsSubstring;

static double matchingFraction(const common::ImageStack &imageStack,
                               const mesh::Mesh3d &mesh3d,
                               const common::VolumeF &voxelSize,
                               const common::VoxelF &originPoint,
                               std::size_t compartmentIndex, QRgb color) {
  // returns fraction of tetrahedra in compartment whose centroid lies in a
  // voxel with this color
  double n_matches{0};
  double n_vertices{0};
  for (const auto &tetrahedron :
       mesh3d.getTetrahedronIndices()[compartmentIndex]) {
    // get centroid of tetrahedron in voxel coordinates
    double z = 0;
    double y = 0;
    double x = 0;
    for (auto vertexIndex : tetrahedron) {
      z += 0.25 *
           (mesh3d.getVerticesAsFlatArray()[3 * vertexIndex + 2] -
            originPoint.z) /
           voxelSize.depth();
      y += 0.25 *
           (mesh3d.getVerticesAsFlatArray()[3 * vertexIndex + 1] -
            originPoint.p.y()) /
           voxelSize.height();
      x += 0.25 *
           (mesh3d.getVerticesAsFlatArray()[3 * vertexIndex] -
            originPoint.p.x()) /
           voxelSize.width();
    }
    if (imageStack[std::clamp(static_cast<std::size_t>(z), std::size_t{0},
                              imageStack.volume().depth())]
            .pixel(static_cast<int>(x),
                   static_cast<int>(imageStack.volume().height() - 1 - y)) ==
        color) {
      ++n_matches;
    }
    ++n_vertices;
  }
  return n_matches / n_vertices;
}

TEST_CASE("Mesh3d simple geometries",
          "[core/mesh/mesh3d][core/mesh][core][mesh3d]") {
  SECTION("empty imageStack") {
    mesh::Mesh3d mesh3d({}, {}, {}, {}, {});
    REQUIRE(mesh3d.isValid() == false);
    REQUIRE_THAT(mesh3d.getErrorMessage(),
                 ContainsSubstring("geometry image missing"));
  }
  SECTION("compartments assigned invalid colours") {
    common::Volume volume(16, 21, 5);
    common::VolumeF voxelSize(1.0, 1.0, 1.0);
    common::VoxelF originPoint(0.0, 0.0, 0.0);
    common::ImageStack imageStack(volume, QImage::Format_RGB32);
    QRgb col = QColor(12, 66, 33).rgb();
    imageStack.fill(col);
    for (const auto &colourVector :
         {std::vector<QRgb>{98765432}, std::vector<QRgb>{58642314, 0}}) {
      mesh::Mesh3d mesh3d(imageStack, {5}, voxelSize, originPoint,
                          colourVector);
      REQUIRE(mesh3d.isValid() == false);
      REQUIRE_THAT(mesh3d.getErrorMessage(),
                   ContainsSubstring("color is not present"));
    }
  }
  SECTION("cuboid") {
    common::Volume volume(16, 21, 5);
    common::VolumeF voxelSize(1.0, 1.0, 1.0);
    common::VoxelF originPoint(0.0, 0.0, 0.0);
    common::ImageStack imageStack(volume, QImage::Format_RGB32);
    QRgb col = 0xff318399;
    imageStack.fill(col);
    std::vector<QRgb> colours{col};
    mesh::Mesh3d mesh3d(imageStack, {3}, voxelSize, originPoint, colours);
    REQUIRE(mesh3d.isValid() == true);
    REQUIRE(mesh3d.getErrorMessage().empty());
    SECTION("check outputs") {
      // vertices
      const auto &vertices = mesh3d.getVerticesAsFlatArray();
      REQUIRE(vertices.size() % 3 == 0); // each vertex has 3 points
      REQUIRE(vertices.size() >= 3 * 8); // need at least 8 vertices

      // tetrahedra
      const auto &tetrahedra = mesh3d.getTetrahedronIndices();
      REQUIRE(tetrahedra.size() == 1);       // 1 compartment
      REQUIRE(tetrahedra[0].size() >= 2);    // some number of tetrahedra
      REQUIRE(tetrahedra[0][0].size() == 4); // each tetrahedron has 4 indices

      // gmsh output
      QStringList msh = mesh3d.getGMSH().split("\n");
      REQUIRE(msh[0] == "$MeshFormat");
      REQUIRE(msh[1] == "2.2 0 8");
      REQUIRE(msh[2] == "$EndMeshFormat");
      REQUIRE(msh[3] == "$Nodes");
      REQUIRE(msh[4] == QString("%1").arg(vertices.size() / 3));
      auto nodeline = msh[5].split(" ");
      REQUIRE(nodeline[0] == "1");
      REQUIRE(nodeline.size() == 4);
      auto elementline = msh[msh.size() - 3].split(" ");
      REQUIRE(elementline[0] == QString("%1").arg(tetrahedra[0].size()));
      REQUIRE(elementline[1] == "4"); // tetrahedron id
      REQUIRE(elementline[2] == "2"); // 2 tags
      REQUIRE(elementline.size() == 9);
      REQUIRE(msh[msh.size() - 2] == "$EndElements");
      REQUIRE(matchingFraction(imageStack, mesh3d, voxelSize, originPoint, 0,
                               col) == dbl_approx(1.0));
    }
    SECTION("reducing max cell volume increases number of cells") {
      auto n_prev = mesh3d.getTetrahedronIndices()[0].size();
      for (auto maxCellVolume : {2, 1}) {
        CAPTURE(maxCellVolume);
        mesh3d.setCompartmentMaxCellVolume(0, maxCellVolume);
        auto n = mesh3d.getTetrahedronIndices()[0].size();
        REQUIRE(mesh3d.getCompartmentMaxCellVolume().size() == 1);
        REQUIRE(mesh3d.getCompartmentMaxCellVolume()[0] == maxCellVolume);
        REQUIRE(mesh3d.getCompartmentMaxCellVolume(0) == maxCellVolume);
        REQUIRE(n > n_prev);
        n_prev = n;
      }
    }
    SECTION("increasing max cell volume decreases number of cells") {
      auto n_prev = mesh3d.getTetrahedronIndices()[0].size();
      for (auto maxCellVolume : {4, 5, 8, 20}) {
        CAPTURE(maxCellVolume);
        mesh3d.setCompartmentMaxCellVolume(0, maxCellVolume);
        auto n = mesh3d.getTetrahedronIndices()[0].size();
        REQUIRE(mesh3d.getCompartmentMaxCellVolume().size() == 1);
        REQUIRE(mesh3d.getCompartmentMaxCellVolume()[0] == maxCellVolume);
        REQUIRE(mesh3d.getCompartmentMaxCellVolume(0) == maxCellVolume);
        REQUIRE(n < n_prev);
        n_prev = n;
      }
    }
    SECTION("modify voxel size keeping aspect ratio") {
      auto nCells =
          static_cast<double>(mesh3d.getTetrahedronIndices()[0].size());
      CAPTURE(nCells);
      constexpr double tolerance = 2.0;
      constexpr double maxCoord = 21.0;
      REQUIRE(std::fabs(common::min(mesh3d.getVerticesAsFlatArray())) <=
              tolerance);
      REQUIRE(std::fabs(common::max(mesh3d.getVerticesAsFlatArray()) -
                        maxCoord) <= tolerance);
      // rescaling without change in aspect ratio
      for (auto scaling : {0.07, 0.13, 0.68, 2.2, 5.5, 10.8, 66.3}) {
        CAPTURE(scaling);
        mesh3d.setPhysicalGeometry({scaling, scaling, scaling}, {0, 0, 0});
        QFile qf(QString("x%1.msh").arg(scaling));
        qf.open(QIODevice::WriteOnly | QIODevice::Text);
        qf.write(mesh3d.getGMSH().toUtf8());
        qf.close();
        REQUIRE(std::fabs(common::min(mesh3d.getVerticesAsFlatArray())) <=
                tolerance * scaling);
        REQUIRE(std::fabs(common::max(mesh3d.getVerticesAsFlatArray()) -
                          maxCoord * scaling) <= tolerance * scaling);
        // meshing should not be significantly affected, just a coordinate
        // rescaling, number of cells should be similar
        auto nCellsNew =
            static_cast<double>(mesh3d.getTetrahedronIndices()[0].size());
        CAPTURE(nCellsNew);
        double relativeDiff = (nCellsNew - nCells) / nCells;
        REQUIRE(std::fabs(relativeDiff) <= 0.10);
      }
    }
    SECTION("modify origin") {
      auto nCells =
          static_cast<double>(mesh3d.getTetrahedronIndices()[0].size());
      CAPTURE(nCells);
      // volume = (16, 21, 5)
      constexpr double tolerance = 2.0;
      constexpr double maxCoord = 21.0;
      REQUIRE(std::fabs(common::min(mesh3d.getVerticesAsFlatArray())) <=
              tolerance);
      REQUIRE(std::fabs(common::max(mesh3d.getVerticesAsFlatArray()) -
                        maxCoord) <= tolerance);
      mesh3d.setPhysicalGeometry({1.0, 1.0, 1.0}, {-12, -5, 77});
      REQUIRE(std::fabs(common::min(mesh3d.getVerticesAsFlatArray()) + 12.0) <=
              tolerance);
      REQUIRE(std::fabs(common::max(mesh3d.getVerticesAsFlatArray()) - 82.0) <=
              tolerance);
      // meshing should not be significantly affected, just a coordinate
      // shift, number of cells should be similar
      auto nCellsNew =
          static_cast<double>(mesh3d.getTetrahedronIndices()[0].size());
      CAPTURE(nCellsNew);
      double relativeDiff = (nCellsNew - nCells) / nCells;
      REQUIRE(std::fabs(relativeDiff) <= 0.10);
    }
    SECTION("modify voxel size and aspect ratio") {
      auto nCells =
          static_cast<double>(mesh3d.getTetrahedronIndices()[0].size());
      CAPTURE(nCells);
      constexpr double tolerance = 2.0;
      constexpr double maxCoord = 21.0;
      REQUIRE(std::fabs(common::min(mesh3d.getVerticesAsFlatArray())) <=
              tolerance);
      REQUIRE(std::fabs(common::max(mesh3d.getVerticesAsFlatArray()) -
                        maxCoord) <= tolerance);
      // rescaling of only the y-dimension of the voxel
      for (auto scaling : {2.9, 5.5, 10.8, 66.3}) {
        CAPTURE(scaling);
        double newMaxCoord = maxCoord * scaling;
        mesh3d.setPhysicalGeometry({1.0, scaling, 1.0}, {0, 0, 0});
        QFile qf(QString("x%1.msh").arg(scaling));
        qf.open(QIODevice::WriteOnly | QIODevice::Text);
        qf.write(mesh3d.getGMSH().toUtf8());
        qf.close();
        REQUIRE(std::fabs(common::min(mesh3d.getVerticesAsFlatArray())) <=
                tolerance * scaling);
        REQUIRE(std::fabs(common::max(mesh3d.getVerticesAsFlatArray()) -
                          newMaxCoord) <= tolerance * scaling);
        // number of cells should scale approx linearly with y-dimension, since
        // y is already the largest dimension of the mesh and the max cell size
        // is determined by the smaller voxel sides hence remains fixed
        auto nCellsNew =
            static_cast<double>(mesh3d.getTetrahedronIndices()[0].size());
        CAPTURE(nCellsNew);
        double increaseInCells = nCellsNew / nCells;
        CAPTURE(increaseInCells / scaling);
        REQUIRE(std::fabs(increaseInCells / scaling - 1.0) <= 0.05);
      }
    }
  }
}

TEST_CASE("Mesh3d more complex geometries",
          "[core/mesh/mesh3d][core/mesh][core][mesh3d][expensive]") {
  SECTION("Single egg") {
    sme::test::createBinaryFile("geometry/3d_single_egg.tiff", "tmp_egg.tif");
    common::TiffReader tiffReader(
        QDir::current().filePath("tmp_egg.tif").toStdString());
    REQUIRE(tiffReader.empty() == false);
    REQUIRE(tiffReader.getErrorMessage().isEmpty());
    auto imageStack = tiffReader.getImages();
    REQUIRE(imageStack.volume().width() == 40);
    REQUIRE(imageStack.volume().height() == 40);
    REQUIRE(imageStack.volume().depth() == 40);
    imageStack.convertToIndexed();
    common::VolumeF voxelSize(1.0, 1.0, 1.0);
    common::VoxelF originPoint(0.0, 0.0, 0.0);
    auto colours = imageStack[0].colorTable();
    REQUIRE(colours.size() == 3);
    QRgb colOutside{0xff000000};
    QRgb colCell{0xffffffff};
    QRgb colNucleus{0xff7f7f7f};
    std::vector<std::size_t> maxCellVolume{5};
    SECTION("No compartment colours") {
      mesh::Mesh3d mesh3d(imageStack, maxCellVolume, voxelSize, originPoint,
                          {});
      REQUIRE(mesh3d.isValid() == false);
    }
    SECTION("Invalid compartment colours") {
      mesh::Mesh3d mesh3d(imageStack, maxCellVolume, voxelSize, originPoint,
                          {0x123456, 0x987654});
      REQUIRE(mesh3d.isValid() == false);
    }
    SECTION("Outside only") {
      mesh::Mesh3d mesh3d(imageStack, maxCellVolume, voxelSize, originPoint,
                          {colOutside});
      REQUIRE(mesh3d.isValid() == true);
      REQUIRE(mesh3d.getErrorMessage().empty());
      REQUIRE(mesh3d.getTetrahedronIndices().size() == 1);
      REQUIRE(matchingFraction(imageStack, mesh3d, voxelSize, originPoint, 0,
                               colOutside) >= 0.98);
      // ok to have a little overlap with cell
      REQUIRE(matchingFraction(imageStack, mesh3d, voxelSize, originPoint, 0,
                               colCell) <= 0.02);
      // should be zero overlap with nucleus
      REQUIRE(matchingFraction(imageStack, mesh3d, voxelSize, originPoint, 0,
                               colNucleus) == dbl_approx(0));
    }
    SECTION("Cell only") {
      mesh::Mesh3d mesh3d(imageStack, maxCellVolume, voxelSize, originPoint,
                          {colCell});
      REQUIRE(mesh3d.isValid() == true);
      REQUIRE(mesh3d.getErrorMessage().empty());
      REQUIRE(mesh3d.getTetrahedronIndices().size() == 1);
      REQUIRE(matchingFraction(imageStack, mesh3d, voxelSize, originPoint, 0,
                               colOutside) <= 0.02);
      REQUIRE(matchingFraction(imageStack, mesh3d, voxelSize, originPoint, 0,
                               colCell) >= 0.90);
      REQUIRE(matchingFraction(imageStack, mesh3d, voxelSize, originPoint, 0,
                               colNucleus) <= 0.08);
    }
    SECTION("Nucleus only") {
      mesh::Mesh3d mesh3d(imageStack, maxCellVolume, voxelSize, originPoint,
                          {colNucleus});
      REQUIRE(mesh3d.isValid() == true);
      REQUIRE(mesh3d.getErrorMessage().empty());
      REQUIRE(mesh3d.getTetrahedronIndices().size() == 1);
      REQUIRE(matchingFraction(imageStack, mesh3d, voxelSize, originPoint, 0,
                               colOutside) == dbl_approx(0));
      REQUIRE(matchingFraction(imageStack, mesh3d, voxelSize, originPoint, 0,
                               colCell) <= 0.05);
      REQUIRE(matchingFraction(imageStack, mesh3d, voxelSize, originPoint, 0,
                               colNucleus) >= 0.95);
    }
    SECTION("Nucleus+Cell only") {
      mesh::Mesh3d mesh3d(imageStack, maxCellVolume, voxelSize, originPoint,
                          {colNucleus, colCell});
      REQUIRE(mesh3d.isValid() == true);
      REQUIRE(mesh3d.getErrorMessage().empty());
      REQUIRE(mesh3d.getTetrahedronIndices().size() == 2);
      // nucleus
      REQUIRE(matchingFraction(imageStack, mesh3d, voxelSize, originPoint, 0,
                               colOutside) == dbl_approx(0));
      REQUIRE(matchingFraction(imageStack, mesh3d, voxelSize, originPoint, 0,
                               colCell) <= 0.05);
      REQUIRE(matchingFraction(imageStack, mesh3d, voxelSize, originPoint, 0,
                               colNucleus) >= 0.95);
      // cell
      REQUIRE(matchingFraction(imageStack, mesh3d, voxelSize, originPoint, 1,
                               colOutside) <= 0.02);
      REQUIRE(matchingFraction(imageStack, mesh3d, voxelSize, originPoint, 1,
                               colCell) >= 0.90);
      REQUIRE(matchingFraction(imageStack, mesh3d, voxelSize, originPoint, 1,
                               colNucleus) <= 0.08);
    }
    SECTION("Nucleus+Outside only") {
      mesh::Mesh3d mesh3d(imageStack, maxCellVolume, voxelSize, originPoint,
                          {colNucleus, colOutside});
      REQUIRE(mesh3d.isValid() == true);
      REQUIRE(mesh3d.getErrorMessage().empty());
      REQUIRE(mesh3d.getTetrahedronIndices().size() == 2);
      // nucleus
      REQUIRE(matchingFraction(imageStack, mesh3d, voxelSize, originPoint, 0,
                               colOutside) == dbl_approx(0));
      REQUIRE(matchingFraction(imageStack, mesh3d, voxelSize, originPoint, 0,
                               colCell) <= 0.05);
      REQUIRE(matchingFraction(imageStack, mesh3d, voxelSize, originPoint, 0,
                               colNucleus) >= 0.95);
      // outside
      REQUIRE(matchingFraction(imageStack, mesh3d, voxelSize, originPoint, 1,
                               colOutside) >= 0.95);
      REQUIRE(matchingFraction(imageStack, mesh3d, voxelSize, originPoint, 1,
                               colCell) <= 0.05);
      REQUIRE(matchingFraction(imageStack, mesh3d, voxelSize, originPoint, 1,
                               colNucleus) == dbl_approx(0));
    }
    SECTION("Cell+Outside only") {
      mesh::Mesh3d mesh3d(imageStack, maxCellVolume, voxelSize, originPoint,
                          {colOutside, colCell});
      REQUIRE(mesh3d.isValid() == true);
      REQUIRE(mesh3d.getErrorMessage().empty());
      REQUIRE(mesh3d.getTetrahedronIndices().size() == 2);
      // outside
      REQUIRE(matchingFraction(imageStack, mesh3d, voxelSize, originPoint, 0,
                               colOutside) >= 0.95);
      REQUIRE(matchingFraction(imageStack, mesh3d, voxelSize, originPoint, 0,
                               colCell) <= 0.05);
      REQUIRE(matchingFraction(imageStack, mesh3d, voxelSize, originPoint, 0,
                               colNucleus) == dbl_approx(0));
      // cell
      REQUIRE(matchingFraction(imageStack, mesh3d, voxelSize, originPoint, 1,
                               colOutside) <= 0.02);
      REQUIRE(matchingFraction(imageStack, mesh3d, voxelSize, originPoint, 1,
                               colCell) >= 0.90);
      REQUIRE(matchingFraction(imageStack, mesh3d, voxelSize, originPoint, 1,
                               colNucleus) <= 0.08);
    }
    SECTION("All three compartments") {
      mesh::Mesh3d mesh3d(imageStack, maxCellVolume, voxelSize, originPoint,
                          sme::common::toStdVec(colours));
      REQUIRE(mesh3d.isValid() == true);
      REQUIRE(mesh3d.getErrorMessage().empty());
      REQUIRE(mesh3d.getTetrahedronIndices().size() == 3);
      for (auto compartmentIndex : {0, 1, 2}) {
        CAPTURE(compartmentIndex);
        REQUIRE(matchingFraction(imageStack, mesh3d, voxelSize, originPoint,
                                 compartmentIndex,
                                 colours[compartmentIndex]) >= 0.90);
      }
    }
  }
  SECTION("Two disconnected eggs") {
    sme::test::createBinaryFile("geometry/3d_two_eggs_disconnected.tiff",
                                "tmp_two_eggs.tif");
    common::TiffReader tiffReader(
        QDir::current().filePath("tmp_two_eggs.tif").toStdString());
    REQUIRE(tiffReader.empty() == false);
    REQUIRE(tiffReader.getErrorMessage().isEmpty());
    auto imageStack = tiffReader.getImages();
    REQUIRE(imageStack.volume().width() == 40);
    REQUIRE(imageStack.volume().height() == 40);
    REQUIRE(imageStack.volume().depth() == 40);
    imageStack.convertToIndexed();
    common::VolumeF voxelSize(1.0, 1.0, 1.0);
    common::VoxelF originPoint(0.0, 0.0, 0.0);
    auto colours = imageStack[0].colorTable();
    REQUIRE(colours.size() == 3);
    QRgb colOutside{0xff000000};
    QRgb colNucleus{0xff7f7f7f};
    QRgb colCell{0xffffffff};
    std::size_t maxCellVolume = 5;
    SECTION("No compartment colours") {
      mesh::Mesh3d mesh3d(imageStack, {maxCellVolume}, voxelSize, originPoint,
                          {});
      REQUIRE(mesh3d.isValid() == false);
    }
    SECTION("Outside only") {
      mesh::Mesh3d mesh3d(imageStack, {maxCellVolume}, voxelSize, originPoint,
                          {colOutside});
      REQUIRE(mesh3d.isValid() == true);
      REQUIRE(mesh3d.getErrorMessage().empty());
      REQUIRE(mesh3d.getTetrahedronIndices().size() == 1);
      REQUIRE(matchingFraction(imageStack, mesh3d, voxelSize, originPoint, 0,
                               colOutside) >= 0.99);
      REQUIRE(matchingFraction(imageStack, mesh3d, voxelSize, originPoint, 0,
                               colCell) <= 0.01);
      REQUIRE(matchingFraction(imageStack, mesh3d, voxelSize, originPoint, 0,
                               colNucleus) == dbl_approx(0));
    }
    SECTION("Cell only") {
      mesh::Mesh3d mesh3d(imageStack, {maxCellVolume}, voxelSize, originPoint,
                          {colCell});
      REQUIRE(mesh3d.isValid() == true);
      REQUIRE(mesh3d.getErrorMessage().empty());
      REQUIRE(mesh3d.getTetrahedronIndices().size() == 1);
      REQUIRE(matchingFraction(imageStack, mesh3d, voxelSize, originPoint, 0,
                               colOutside) <= 0.08);
      REQUIRE(matchingFraction(imageStack, mesh3d, voxelSize, originPoint, 0,
                               colCell) >= 0.90);
      REQUIRE(matchingFraction(imageStack, mesh3d, voxelSize, originPoint, 0,
                               colNucleus) <= 0.08);
    }
    SECTION("Nucleus only") {
      mesh::Mesh3d mesh3d(imageStack, {maxCellVolume}, voxelSize, originPoint,
                          {colNucleus});
      REQUIRE(mesh3d.isValid() == true);
      REQUIRE(mesh3d.getErrorMessage().empty());
      REQUIRE(mesh3d.getTetrahedronIndices().size() == 1);
      REQUIRE(matchingFraction(imageStack, mesh3d, voxelSize, originPoint, 0,
                               colOutside) == dbl_approx(0));
      REQUIRE(matchingFraction(imageStack, mesh3d, voxelSize, originPoint, 0,
                               colCell) <= 0.25);
      REQUIRE(matchingFraction(imageStack, mesh3d, voxelSize, originPoint, 0,
                               colNucleus) >= 0.75);
    }
    SECTION("Nucleus+Cell only") {
      mesh::Mesh3d mesh3d(imageStack, {maxCellVolume, maxCellVolume}, voxelSize,
                          originPoint, {colNucleus, colCell});
      REQUIRE(mesh3d.isValid() == true);
      REQUIRE(mesh3d.getErrorMessage().empty());
      REQUIRE(mesh3d.getTetrahedronIndices().size() == 2);
      REQUIRE(matchingFraction(imageStack, mesh3d, voxelSize, originPoint, 0,
                               colNucleus) >= 0.75);
      REQUIRE(matchingFraction(imageStack, mesh3d, voxelSize, originPoint, 1,
                               colCell) >= 0.75);
    }
    SECTION("Nucleus+Outside only") {
      mesh::Mesh3d mesh3d(imageStack, {maxCellVolume, maxCellVolume}, voxelSize,
                          originPoint, {colNucleus, colOutside});
      REQUIRE(mesh3d.isValid() == true);
      REQUIRE(mesh3d.getErrorMessage().empty());
      REQUIRE(mesh3d.getTetrahedronIndices().size() == 2);
      REQUIRE(matchingFraction(imageStack, mesh3d, voxelSize, originPoint, 0,
                               colNucleus) >= 0.75);
      REQUIRE(matchingFraction(imageStack, mesh3d, voxelSize, originPoint, 1,
                               colOutside) >= 0.75);
    }
    SECTION("Cell+Outside only") {
      mesh::Mesh3d mesh3d(imageStack, {maxCellVolume, maxCellVolume}, voxelSize,
                          originPoint, {colOutside, colCell});
      REQUIRE(mesh3d.isValid() == true);
      REQUIRE(mesh3d.getErrorMessage().empty());
      REQUIRE(mesh3d.getTetrahedronIndices().size() == 2);
      REQUIRE(matchingFraction(imageStack, mesh3d, voxelSize, originPoint, 0,
                               colOutside) >= 0.90);
      REQUIRE(matchingFraction(imageStack, mesh3d, voxelSize, originPoint, 1,
                               colCell) >= 0.90);
    }
    SECTION("All three compartments") {
      mesh::Mesh3d mesh3d(
          imageStack, {maxCellVolume, maxCellVolume, maxCellVolume}, voxelSize,
          originPoint, sme::common::toStdVec(colours));
      REQUIRE(mesh3d.isValid() == true);
      REQUIRE(mesh3d.getErrorMessage().empty());
      REQUIRE(mesh3d.getTetrahedronIndices().size() == 3);
      REQUIRE(matchingFraction(imageStack, mesh3d, voxelSize, originPoint, 0,
                               colours[0]) >= 0.98);
      REQUIRE(matchingFraction(imageStack, mesh3d, voxelSize, originPoint, 1,
                               colours[1]) >= 0.85);
      REQUIRE(matchingFraction(imageStack, mesh3d, voxelSize, originPoint, 2,
                               colours[2]) >= 0.75);
    }
  }
  SECTION("Hollow sphere") {
    auto m = sme::test::getTestModel("hollow-sphere-3d");
    auto &geometry = m.getGeometry();
    auto *compartment = m.getCompartments().getCompartment("Nucleus");
    REQUIRE(compartment != nullptr);
    auto voxelSize = geometry.getVoxelSize();
    auto originPoint = geometry.getPhysicalOrigin();
    REQUIRE(geometry.getHasImage() == true);
    auto imageStack = geometry.getImages();
    REQUIRE(imageStack.volume().width() == 50);
    REQUIRE(imageStack.volume().height() == 50);
    REQUIRE(imageStack.volume().depth() == 50);
    auto colorTable = imageStack[0].colorTable();
    REQUIRE(colorTable.size() == 2);
    QRgb colOutside = 0xffffe119;
    QRgb colInside = compartment->getColour();
    REQUIRE(colorTable[0] == colOutside);
    REQUIRE(colorTable[1] == colInside);
    REQUIRE(geometry.getIsMeshValid() == true);
    auto mesh3d = geometry.getMesh3d();
    REQUIRE(mesh3d != nullptr);
    mesh3d->setCompartmentMaxCellVolume(0, 5);
    // single compartment
    REQUIRE(mesh3d->getTetrahedronIndices().size() == 1);
    REQUIRE(matchingFraction(imageStack, *mesh3d, voxelSize, originPoint, 0,
                             colInside) >= 0.90);
    REQUIRE(matchingFraction(imageStack, *mesh3d, voxelSize, originPoint, 0,
                             colOutside) <= 0.10);
    // assign compartment to outside colour
    m.getCompartments().setColour("Nucleus", colOutside);
    mesh3d = geometry.getMesh3d();
    REQUIRE(mesh3d != nullptr);
    REQUIRE(mesh3d->getTetrahedronIndices().size() == 1);
    REQUIRE(matchingFraction(imageStack, *mesh3d, voxelSize, originPoint, 0,
                             colOutside) >= 0.99);
    REQUIRE(matchingFraction(imageStack, *mesh3d, voxelSize, originPoint, 0,
                             colInside) <= 0.01);
  }
}

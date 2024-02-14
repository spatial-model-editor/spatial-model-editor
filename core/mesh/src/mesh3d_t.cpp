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

using namespace sme;
using Catch::Matchers::ContainsSubstring;

TEST_CASE("Mesh3d", "[core/mesh/mesh3d][core/mesh][core][mesh3d]") {
  SECTION("empty imageStack") {
    mesh::Mesh3d mesh3d({}, {}, {}, {}, {});
    REQUIRE(mesh3d.isValid() == false);
    REQUIRE_THAT(mesh3d.getErrorMessage(),
                 ContainsSubstring("geometry image missing"));
  }
  SECTION("no compartment colours") {
    common::Volume volume(16, 21, 5);
    common::VolumeF voxelSize(1.0, 1.0, 1.0);
    common::VoxelF originPoint(0.0, 0.0, 0.0);
    common::ImageStack imageStack(volume, QImage::Format_RGB32);
    QRgb col = QColor(12, 66, 33).rgb();
    imageStack.fill(col);
    for (const auto &colourVector :
         {std::vector<QRgb>{}, std::vector<QRgb>{98765432},
          std::vector<QRgb>{58642314, 0}}) {
      mesh::Mesh3d mesh3d(imageStack, {5}, voxelSize, originPoint,
                          colourVector);
      REQUIRE(mesh3d.isValid() == false);
      REQUIRE_THAT(
          mesh3d.getErrorMessage(),
          ContainsSubstring("No compartments have been assigned a colour"));
    }
  }
  SECTION("cube") {
    common::Volume volume(16, 21, 5);
    common::VolumeF voxelSize(1.0, 1.0, 1.0);
    common::VoxelF originPoint(0.0, 0.0, 0.0);
    common::ImageStack imageStack(volume, QImage::Format_RGB32);
    QRgb col = QColor(0, 0, 0).rgb();
    imageStack.fill(col);
    std::vector<QRgb> colours{col};
    mesh::Mesh3d mesh3d(imageStack, {10}, voxelSize, originPoint, colours);
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
    }
  }
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
    QRgb colCell{0xff7f7f7f};
    QRgb colNucleus{0xffffffff};
    std::vector<std::size_t> maxCellVolume{5};
    SECTION("No compartment colours") {
      mesh::Mesh3d mesh3d(imageStack, maxCellVolume, voxelSize, originPoint,
                          {});
      REQUIRE(mesh3d.isValid() == false);
    }
    SECTION("Outside only") {
      mesh::Mesh3d mesh3d(imageStack, maxCellVolume, voxelSize, originPoint,
                          {colOutside});
      REQUIRE(mesh3d.isValid() == true);
      REQUIRE(mesh3d.getErrorMessage().empty());
      REQUIRE(mesh3d.getTetrahedronIndices().size() == 1);
    }
    SECTION("Cell only") {
      mesh::Mesh3d mesh3d(imageStack, maxCellVolume, voxelSize, originPoint,
                          {colCell});
      REQUIRE(mesh3d.isValid() == true);
      REQUIRE(mesh3d.getErrorMessage().empty());
      REQUIRE(mesh3d.getTetrahedronIndices().size() == 1);
    }
    SECTION("Nucleus only") {
      mesh::Mesh3d mesh3d(imageStack, maxCellVolume, voxelSize, originPoint,
                          {colNucleus});
      REQUIRE(mesh3d.isValid() == true);
      REQUIRE(mesh3d.getErrorMessage().empty());
      REQUIRE(mesh3d.getTetrahedronIndices().size() == 1);
    }
    SECTION("Nucleus+Cell only") {
      mesh::Mesh3d mesh3d(imageStack, maxCellVolume, voxelSize, originPoint,
                          {colNucleus, colCell});
      REQUIRE(mesh3d.isValid() == true);
      REQUIRE(mesh3d.getErrorMessage().empty());
      REQUIRE(mesh3d.getTetrahedronIndices().size() == 2);
    }
    SECTION("Nucleus+Outside only") {
      mesh::Mesh3d mesh3d(imageStack, maxCellVolume, voxelSize, originPoint,
                          {colNucleus, colOutside});
      REQUIRE(mesh3d.isValid() == true);
      REQUIRE(mesh3d.getErrorMessage().empty());
      REQUIRE(mesh3d.getTetrahedronIndices().size() == 2);
    }
    SECTION("Cell+Outside only") {
      mesh::Mesh3d mesh3d(imageStack, maxCellVolume, voxelSize, originPoint,
                          {colOutside, colCell});
      REQUIRE(mesh3d.isValid() == true);
      REQUIRE(mesh3d.getErrorMessage().empty());
      REQUIRE(mesh3d.getTetrahedronIndices().size() == 2);
    }
    SECTION("All three compartments") {
      mesh::Mesh3d mesh3d(imageStack, maxCellVolume, voxelSize, originPoint,
                          sme::common::toStdVec(colours));
      REQUIRE(mesh3d.isValid() == true);
      REQUIRE(mesh3d.getErrorMessage().empty());
      REQUIRE(mesh3d.getTetrahedronIndices().size() == 3);
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
    QRgb colCell{0xff7f7f7f};
    QRgb colNucleus{0xffffffff};
    std::vector<std::size_t> maxCellVolume{3};
    SECTION("No compartment colours") {
      mesh::Mesh3d mesh3d(imageStack, maxCellVolume, voxelSize, originPoint,
                          {});
      REQUIRE(mesh3d.isValid() == false);
    }
    SECTION("Outside only") {
      mesh::Mesh3d mesh3d(imageStack, maxCellVolume, voxelSize, originPoint,
                          {colOutside});
      REQUIRE(mesh3d.isValid() == true);
      REQUIRE(mesh3d.getErrorMessage().empty());
      REQUIRE(mesh3d.getTetrahedronIndices().size() == 1);
    }
    SECTION("Cell only") {
      mesh::Mesh3d mesh3d(imageStack, maxCellVolume, voxelSize, originPoint,
                          {colCell});
      REQUIRE(mesh3d.isValid() == true);
      REQUIRE(mesh3d.getErrorMessage().empty());
      REQUIRE(mesh3d.getTetrahedronIndices().size() == 1);
    }
    SECTION("Nucleus only") {
      mesh::Mesh3d mesh3d(imageStack, maxCellVolume, voxelSize, originPoint,
                          {colNucleus});
      REQUIRE(mesh3d.isValid() == true);
      REQUIRE(mesh3d.getErrorMessage().empty());
      REQUIRE(mesh3d.getTetrahedronIndices().size() == 1);
    }
    SECTION("Nucleus+Cell only") {
      mesh::Mesh3d mesh3d(imageStack, maxCellVolume, voxelSize, originPoint,
                          {colNucleus, colCell});
      REQUIRE(mesh3d.isValid() == true);
      REQUIRE(mesh3d.getErrorMessage().empty());
      REQUIRE(mesh3d.getTetrahedronIndices().size() == 2);
    }
    SECTION("Nucleus+Outside only") {
      mesh::Mesh3d mesh3d(imageStack, maxCellVolume, voxelSize, originPoint,
                          {colNucleus, colOutside});
      REQUIRE(mesh3d.isValid() == true);
      REQUIRE(mesh3d.getErrorMessage().empty());
      REQUIRE(mesh3d.getTetrahedronIndices().size() == 2);
    }
    SECTION("Cell+Outside only") {
      mesh::Mesh3d mesh3d(imageStack, maxCellVolume, voxelSize, originPoint,
                          {colOutside, colCell});
      REQUIRE(mesh3d.isValid() == true);
      REQUIRE(mesh3d.getErrorMessage().empty());
      REQUIRE(mesh3d.getTetrahedronIndices().size() == 2);
    }
    SECTION("All three compartments") {
      mesh::Mesh3d mesh3d(imageStack, maxCellVolume, voxelSize, originPoint,
                          sme::common::toStdVec(colours));
      REQUIRE(mesh3d.isValid() == true);
      REQUIRE(mesh3d.getErrorMessage().empty());
      REQUIRE(mesh3d.getTetrahedronIndices().size() == 3);
      QFile f("grid.msh");
      f.open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text);
      f.write(mesh3d.getGMSH().toUtf8());
    }
  }
}

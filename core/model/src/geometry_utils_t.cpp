#include "catch_wrapper.hpp"
#include "sme/geometry_utils.hpp"

using namespace sme;

TEST_CASE(
    "Geometry Utils: Voxels",
    "[core/model/geometry_utils][core/model][core][model][geometry_utils]") {
  SECTION("VoxelIndexer constructors use the same volume and voxel ordering") {
    const common::Volume volume{3, 4, 5};
    const std::vector<common::Voxel> voxels{{2, 3, 4}, {0, 0, 0}};
    const geometry::VoxelIndexer fromVolume{volume, voxels};
    const geometry::VoxelIndexer fromDimensions{3, 4, 5, voxels};
    REQUIRE(fromVolume.size() == voxels.size());
    REQUIRE(fromDimensions.size() == voxels.size());
    for (std::size_t i = 0; i < voxels.size(); ++i) {
      REQUIRE(fromVolume.getIndex(voxels[i]).value() == i);
      REQUIRE(fromDimensions.getIndex(voxels[i]).value() == i);
    }
    REQUIRE_FALSE(fromDimensions.getIndex({1, 1, 1}).has_value());
    REQUIRE_FALSE(fromDimensions.getIndex({3, 0, 0}).has_value());
  }
  SECTION("Large voxel offsets without allocating image data") {
    const geometry::VoxelFlattener deep{1024, 1024, 2049};
    REQUIRE(deep.flatten({0, 0, 2048}) == 2147483648ULL);
    const geometry::VoxelFlattener wide{65536, 65536, 2};
    REQUIRE(wide.flatten({65535, 65535, 1}) == 8589934591ULL);
  }
  SECTION("Empty depth with a plane larger than the signed 32-bit range") {
    const geometry::VoxelIndexer empty{65536, 65536, 0};
    REQUIRE(empty.size() == 0);
    REQUIRE_FALSE(empty.getIndex({0, 0, 0}).has_value());
  }
}

TEST_CASE(
    "Geometry Utils: QPoints",
    "[core/model/geometry_utils][core/model][core][model][geometry_utils]") {
  SECTION("QPointIndexer") {
    QSize size(20, 16);

    std::vector<QPoint> v{QPoint(1, 3), QPoint(5, 6), QPoint(9, 9)};
    geometry::QPointIndexer qpi(size, v);

    REQUIRE(qpi.getIndex(v[0]).has_value() == true);
    REQUIRE(qpi.getIndex(v[0]).value() == 0);
    REQUIRE(qpi.getIndex(v[1]).has_value() == true);
    REQUIRE(qpi.getIndex(v[1]).value() == 1);
    REQUIRE(qpi.getIndex(v[2]).has_value() == true);
    REQUIRE(qpi.getIndex(v[2]).value() == 2);

    REQUIRE(qpi.getIndex(QPoint(0, 0)).has_value() == false);
    REQUIRE(qpi.getIndex(QPoint(11, 11)).has_value() == false);
    REQUIRE(qpi.getIndex(QPoint(18, 4)).has_value() == false);
    REQUIRE(qpi.getIndex(QPoint(-1, -12)).has_value() == false);
    REQUIRE(qpi.getIndex(QPoint(38, 46)).has_value() == false);

    qpi.addPoints({QPoint(0, 0), QPoint(11, 11)});
    REQUIRE(qpi.getIndex(QPoint(0, 0)).has_value() == true);
    REQUIRE(qpi.getIndex(QPoint(0, 0)).value() == 3);
    REQUIRE(qpi.getIndex(QPoint(11, 11)).has_value() == true);
    REQUIRE(qpi.getIndex(QPoint(11, 11)).value() == 4);

    REQUIRE_THROWS(qpi.addPoints({QPoint(-1, 4)}));
    REQUIRE_THROWS(qpi.addPoints({QPoint(281, 117)}));
    REQUIRE_THROWS(qpi.addPoints({QPoint(20, 16)}));

    qpi = geometry::QPointIndexer(QSize(99, 99));
    REQUIRE(qpi.getIndex(v[0]).has_value() == false);
    qpi.addPoints(v);
    REQUIRE(qpi.getIndex(v[0]).has_value() == true);
  }
  SECTION("QPointUniqueIndexer") {
    QSize size(20, 16);
    std::vector<QPoint> v{QPoint(1, 3), QPoint(5, 6), QPoint(9, 9)};
    geometry::QPointUniqueIndexer qpi(size, v);
    REQUIRE(qpi.getPoints().size() == 3);
    REQUIRE(qpi.getPoints() == v);

    REQUIRE(qpi.getIndex(v[0]).has_value() == true);
    REQUIRE(qpi.getIndex(v[0]).value() == 0);
    REQUIRE(qpi.getIndex(v[1]).has_value() == true);
    REQUIRE(qpi.getIndex(v[1]).value() == 1);
    REQUIRE(qpi.getIndex(v[2]).has_value() == true);
    REQUIRE(qpi.getIndex(v[2]).value() == 2);

    REQUIRE(qpi.getIndex(QPoint(0, 0)).has_value() == false);
    REQUIRE(qpi.getIndex(QPoint(11, 11)).has_value() == false);
    REQUIRE(qpi.getIndex(QPoint(18, 4)).has_value() == false);
    REQUIRE(qpi.getIndex(QPoint(-1, -12)).has_value() == false);
    REQUIRE(qpi.getIndex(QPoint(38, 46)).has_value() == false);

    std::vector<QPoint> v2{QPoint(1, 3), QPoint(11, 11), QPoint(1, 3)};
    qpi.addPoints(v2);
    REQUIRE(qpi.getPoints().size() == 4);
    REQUIRE(qpi.getIndex(v[0]).value() == 0);
    REQUIRE(qpi.getIndex(QPoint(11, 11)).value() == 3);
    qpi.addPoints(v2);
    REQUIRE(qpi.getPoints().size() == 4);

    REQUIRE_THROWS(qpi.addPoints({QPoint(-1, 4)}));
    REQUIRE_THROWS(qpi.addPoints({QPoint(281, 117)}));
    REQUIRE_THROWS(qpi.addPoints({QPoint(20, 16)}));

    qpi = geometry::QPointUniqueIndexer(QSize(99, 99));
    REQUIRE(qpi.getIndex(v[0]).has_value() == false);
    REQUIRE(qpi.getPoints().size() == 0);
    qpi.addPoints(v);
    REQUIRE(qpi.getIndex(v[0]).has_value() == true);
    REQUIRE(qpi.getIndex(v[0]).value() == 0);
    REQUIRE(qpi.getPoints().size() == v.size());
    qpi.addPoints(v);
    REQUIRE(qpi.getIndex(v[0]).has_value() == true);
    REQUIRE(qpi.getIndex(v[0]).value() == 0);
    REQUIRE(qpi.getPoints().size() == v.size());
  }
}

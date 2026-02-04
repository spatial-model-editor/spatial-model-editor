#include "catch_wrapper.hpp"
#include "sme/voxel.hpp"

using namespace sme;

TEST_CASE("Voxel", "[core/common/voxel][core/common][core][voxel]") {
  SECTION("Voxel") {
    sme::common::Voxel v0(0, 0, 0);
    sme::common::Voxel v1(QPoint(2, 7), 5);
    sme::common::Voxel v2{2, 7, 5};
    REQUIRE(v1 == v2);
    REQUIRE(v1.p == v2.p);
    REQUIRE(v1.z == v2.z);
    auto v3{2 * v1 - v2 - v2};
    REQUIRE(v3 == v0);
    REQUIRE(v3.p.x() == 0);
    REQUIRE(v3.p.y() == 0);
    REQUIRE(v3.z == 0);
  }
  SECTION("toVoxelIndex") {
    REQUIRE(sme::common::toVoxelIndex(0.49, 0.0, 1.0, 5) == 0);
    REQUIRE(sme::common::toVoxelIndex(2.01, 0.0, 1.0, 5) == 2);
    REQUIRE(sme::common::toVoxelIndex(-100.0, 0.0, 1.0, 5) == 0);
    REQUIRE(sme::common::toVoxelIndex(100.0, 0.0, 1.0, 5) == 4);
    REQUIRE(sme::common::toVoxelIndex(100.0, 0.0, 1.0, 1) == 0);
    REQUIRE(sme::common::toVoxelIndex(100.0, 0.0, 1.0, 0) == 0);
    REQUIRE(sme::common::toVoxelIndex(100.0, 0.0, 0.0, 5) == 0);
  }
  SECTION("yIndex") {
    REQUIRE(sme::common::yIndex(1, 6, false) == 1);
    REQUIRE(sme::common::yIndex(1, 6, true) == 4);
  }
  SECTION("voxelArrayIndex") {
    const sme::common::Volume vol{4, 5, 3};
    const sme::common::Voxel voxel{2, 1, 2};
    REQUIRE(sme::common::voxelArrayIndex(vol, voxel, false) ==
            static_cast<std::size_t>(2 + 4 * 1 + 4 * 5 * 2));
    REQUIRE(sme::common::voxelArrayIndex(vol, voxel, true) ==
            static_cast<std::size_t>(2 + 4 * (5 - 1 - 1) + 4 * 5 * 2));
    REQUIRE(sme::common::voxelArrayIndex(vol, 2, 1, 2, true) ==
            sme::common::voxelArrayIndex(vol, voxel, true));
  }
}

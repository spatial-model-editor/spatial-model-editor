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
}

#include "catch_wrapper.hpp"
#include "geometry_impl.hpp"

using namespace sme;

TEST_CASE("fillMissingByDilation", "[core/model/geometry][core/"
                                   "model][core][model][geometry]") {
  int nx = 2;
  int ny = 3;
  int nz = 5;
  std::vector<std::size_t> arr(static_cast<std::size_t>(nx * ny * nz), 6);
  std::size_t invalidIndex = 6;
  SECTION("empty array") {
    std::vector<std::size_t> empty{};
    geometry::fillMissingByDilation(empty, 0, 0, 0, invalidIndex);
    REQUIRE(empty.empty());
  }
  SECTION("single valid voxel") {
    arr[15] = 13;
    geometry::fillMissingByDilation(arr, nx, ny, nz, invalidIndex);
    for (auto a : arr) {
      // every element contains the only valid index
      REQUIRE(a == 13);
    }
  }
  SECTION("no valid voxels") {
    geometry::fillMissingByDilation(arr, nx, ny, nz, invalidIndex);
    for (auto a : arr) {
      // 0 used if no valid voxel is found
      REQUIRE(a == 0);
    }
  }
}

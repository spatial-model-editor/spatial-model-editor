#include "catch_wrapper.hpp"
#include "contour_map.hpp"
#include <QSize>

SCENARIO("ContourMap",
         "[core/mesh/contour_map][core/mesh][core][contour_map]") {
  GIVEN("single contour") {
    auto none = mesh::ContourMap::nullIndex;
    QSize imgSize(10, 10);
    std::vector<cv::Point> c0{{0, 0}, {1, 0}, {2, 0}, {2, 1},
                              {2, 2}, {1, 2}, {0, 2}, {0, 1}};
    std::vector<cv::Point> c1{{0, 3}, {1, 3}, {2, 3}};
    std::vector<cv::Point> c2{{6, 6}, {6, 7}, {7, 8}};
    mesh::ContourMap contourMap(imgSize, {c0, c1, c2});

    // (9,9) not a contour - no neighbouring pixels from other contours
    REQUIRE(contourMap.getContourIndex({9, 9}) == none);
    REQUIRE(contourMap.hasNeighbourWithThisIndex({9, 9}, 0) == false);
    REQUIRE(contourMap.hasNeighbourWithThisIndex({9, 9}, 1) == false);
    REQUIRE(contourMap.hasNeighbourWithThisIndex({9, 9}, 2) == false);
    // all adjacent pixels are null
    REQUIRE(contourMap.hasNeighbourWithThisIndex({9, 9}, none) == true);
    REQUIRE(contourMap.getAdjacentContourIndex({9, 9}) == none);

    // (8,8) not a contour - (7,8) adjacent pixel is from c2
    REQUIRE(contourMap.getContourIndex({8, 8}) == none);
    REQUIRE(contourMap.hasNeighbourWithThisIndex({8, 8}, 0) == false);
    REQUIRE(contourMap.hasNeighbourWithThisIndex({8, 8}, 1) == false);
    REQUIRE(contourMap.hasNeighbourWithThisIndex({8, 8}, 2) == true);
    // not all adjacent pixels are null
    REQUIRE(contourMap.hasNeighbourWithThisIndex({8, 8}, none) == false);
    REQUIRE(contourMap.getAdjacentContourIndex({8, 8}) == 2);

    // (0,0) belongs to c0 - no adjacent pixels from other contours
    REQUIRE(contourMap.getContourIndex({0, 0}) == 0);
    REQUIRE(contourMap.hasNeighbourWithThisIndex({0, 0}, 0) == true);
    REQUIRE(contourMap.hasNeighbourWithThisIndex({0, 0}, 1) == false);
    REQUIRE(contourMap.hasNeighbourWithThisIndex({0, 0}, 2) == false);
    // all adjacent pixels are null (or also from c0)
    REQUIRE(contourMap.hasNeighbourWithThisIndex({0, 0}, none) == true);
    REQUIRE(contourMap.getAdjacentContourIndex({0, 0}) == none);

    // (2,2) belongs to c0 - (2,3) adjacent pixel is from c1
    REQUIRE(contourMap.getContourIndex({2, 2}) == 0);
    REQUIRE(contourMap.hasNeighbourWithThisIndex({2, 2}, 0) == true);
    REQUIRE(contourMap.hasNeighbourWithThisIndex({2, 2}, 1) == true);
    REQUIRE(contourMap.hasNeighbourWithThisIndex({2, 2}, 2) == false);
    REQUIRE(contourMap.hasNeighbourWithThisIndex({2, 2}, none) == false);
    REQUIRE(contourMap.getAdjacentContourIndex({2, 2}) == 1);

    // (2,3) belongs to c1 - (2,2) adjacent pixel is from c0
    REQUIRE(contourMap.getContourIndex({2, 3}) == 1);
    REQUIRE(contourMap.hasNeighbourWithThisIndex({2, 3}, 0) == true);
    REQUIRE(contourMap.hasNeighbourWithThisIndex({2, 3}, 1) == true);
    REQUIRE(contourMap.hasNeighbourWithThisIndex({2, 3}, 2) == false);
    REQUIRE(contourMap.hasNeighbourWithThisIndex({2, 3}, none) == false);
    REQUIRE(contourMap.getAdjacentContourIndex({2, 3}) == 0);

    // (6,7) belongs to c2 - no adjacent pixels from other contours
    REQUIRE(contourMap.getContourIndex({6, 7}) == 2);
    REQUIRE(contourMap.hasNeighbourWithThisIndex({6, 7}, 0) == false);
    REQUIRE(contourMap.hasNeighbourWithThisIndex({6, 7}, 1) == false);
    REQUIRE(contourMap.hasNeighbourWithThisIndex({6, 7}, 2) == true);
    // all adjacent pixels are null (or also from c2)
    REQUIRE(contourMap.hasNeighbourWithThisIndex({6, 7}, none) == true);
    REQUIRE(contourMap.getAdjacentContourIndex({6, 7}) == none);

    // some but not all pixels in c0 have neighbours from another contour
    REQUIRE(contourMap.hasNeighbourWithThisIndex(c0, 1) == false);
    REQUIRE(contourMap.hasNeighbourWithThisIndex(c0, 2) == false);
    REQUIRE(contourMap.hasNeighbourWithThisIndex(c0, none) == false);

    // every pixel of c1 also has a pixel from c0 as a neighbour
    REQUIRE(contourMap.hasNeighbourWithThisIndex(c1, 0) == true);
    REQUIRE(contourMap.hasNeighbourWithThisIndex(c1, 2) == false);
    REQUIRE(contourMap.hasNeighbourWithThisIndex(c1, none) == false);

    // c2 is completely isolated
    REQUIRE(contourMap.hasNeighbourWithThisIndex(c2, 0) == false);
    REQUIRE(contourMap.hasNeighbourWithThisIndex(c2, 1) == false);
    REQUIRE(contourMap.hasNeighbourWithThisIndex(c2, none) == true);
  }
}

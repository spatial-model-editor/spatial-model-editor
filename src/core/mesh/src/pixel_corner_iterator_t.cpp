#include "catch_wrapper.hpp"
#include "pixel_corner_iterator.hpp"

SCENARIO("PixelCornerIterator", "[core/mesh/pixel_corner_iterator][core/"
                                "mesh][core][pixel_corner_iterator]") {
  GIVEN("single pixel outer contour") {
    std::vector<cv::Point> contour{{1, 0}};
    auto pci{mesh::PixelCornerIterator(contour, true)};
    REQUIRE(pci.done() == false);
    REQUIRE(pci.vertex() == cv::Point{1, 0});
    ++pci;
    REQUIRE(pci.done() == false);
    REQUIRE(pci.vertex() == cv::Point{1, 1});
    ++pci;
    REQUIRE(pci.done() == false);
    REQUIRE(pci.vertex() == cv::Point{2, 1});
    ++pci;
    REQUIRE(pci.done() == false);
    REQUIRE(pci.vertex() == cv::Point{2, 0});
    ++pci;
    REQUIRE(pci.done() == true);
    REQUIRE(pci.vertex() == cv::Point{1, 0});
  }
  GIVEN("4-pixel inner contour") {
    // this inner contour has the same edge vertices as a
    // single pixel outer contour at (1,0)
    // but starting at a different vertex and in reverse order
    std::vector<cv::Point> contour{{1, -1}, {2, 0}, {1, 1}, {0, 0}};
    auto pci{mesh::PixelCornerIterator(contour, false)};
    REQUIRE(pci.done() == false);
    REQUIRE(pci.vertex() == cv::Point{2, 0});
    ++pci;
    REQUIRE(pci.done() == false);
    REQUIRE(pci.vertex() == cv::Point{2, 1});
    ++pci;
    REQUIRE(pci.done() == false);
    REQUIRE(pci.vertex() == cv::Point{1, 1});
    ++pci;
    REQUIRE(pci.done() == false);
    REQUIRE(pci.vertex() == cv::Point{1, 0});
    ++pci;
    REQUIRE(pci.done() == true);
    REQUIRE(pci.vertex() == cv::Point{2, 0});
  }
}

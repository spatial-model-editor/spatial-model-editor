#include "catch_wrapper.hpp"
#include "geometry_analytic.hpp"
#include "geometry_sampled_field.hpp"
#include "model_test_utils.hpp"
#include "sme/model.hpp"
#include "sme/utils.hpp"
#include <QFile>
#include <QImage>

using namespace sme;
using namespace sme::test;

TEST_CASE("Analytic geometry", "[core/model/geometry_analytic][core/"
                               "model][core][model][geometry_analytic]") {
  SECTION("SBML model with 2d analytic geometry") {
    auto s{getTestModel("analytic_2d")};
    REQUIRE(s.getGeometry().getIsValid() == true);
    REQUIRE(s.getGeometry().getHasImage() == true);
    const auto &img{s.getGeometry().getImage()};
    REQUIRE(img.colorCount() == 3);
    REQUIRE(img.size() == QSize(200, 200));
    REQUIRE(img.pixel(100, 100) == common::indexedColours()[0].rgb());
    REQUIRE(img.pixel(80, 80) == common::indexedColours()[1].rgb());
    REQUIRE(img.pixel(30, 20) == common::indexedColours()[2].rgb());
  }
  SECTION("SBML model with 3d analytic geometry") {
    auto s{getTestModel("analytic_3d")};
    REQUIRE(s.getGeometry().getIsValid() == true);
    REQUIRE(s.getGeometry().getHasImage() == true);
    const auto &img{s.getGeometry().getImage()};
    REQUIRE(img.colorCount() == 3);
    REQUIRE(img.size() == QSize(200, 200));
    REQUIRE(img.pixel(100, 100) == common::indexedColours()[0].rgb());
    REQUIRE(img.pixel(80, 80) == common::indexedColours()[1].rgb());
    REQUIRE(img.pixel(30, 20) == common::indexedColours()[2].rgb());
  }
}

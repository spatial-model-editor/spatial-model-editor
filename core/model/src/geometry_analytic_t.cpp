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
    const auto &imgs{s.getGeometry().getImages()};
    REQUIRE(imgs[0].colorCount() == 3);
    REQUIRE(imgs[0].size() == QSize(50, 50));
    REQUIRE(imgs[0].pixel(25, 25) == common::indexedColors()[0].rgb());
    REQUIRE(imgs[0].pixel(20, 20) == common::indexedColors()[1].rgb());
    REQUIRE(imgs[0].pixel(8, 5) == common::indexedColors()[2].rgb());
  }
  SECTION("SBML model with 3d analytic geometry") {
    auto s{getTestModel("analytic_3d")};
    REQUIRE(s.getGeometry().getIsValid() == true);
    REQUIRE(s.getGeometry().getHasImage() == true);
    const auto &imgs{s.getGeometry().getImages()};
    REQUIRE(imgs.volume().width() == 50);
    REQUIRE(imgs.volume().height() == 50);
    REQUIRE(imgs.volume().depth() == 50);
    REQUIRE(imgs[0].colorCount() == 3);
    REQUIRE(imgs[0].size() == QSize(50, 50));
    // first z-slice is all background
    REQUIRE(imgs[0].pixel(25, 25) == common::indexedColors()[2].rgb());
    REQUIRE(imgs[0].pixel(20, 20) == common::indexedColors()[2].rgb());
    REQUIRE(imgs[0].pixel(8, 5) == common::indexedColors()[2].rgb());
    // central z-slice has 3 concentric circles
    REQUIRE(imgs[24].pixel(26, 24) == common::indexedColors()[0].rgb());
    REQUIRE(imgs[24].pixel(20, 20) == common::indexedColors()[1].rgb());
    REQUIRE(imgs[24].pixel(7, 3) == common::indexedColors()[2].rgb());
  }
}

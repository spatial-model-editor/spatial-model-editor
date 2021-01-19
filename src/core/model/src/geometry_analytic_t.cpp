#include "catch_wrapper.hpp"
#include "geometry_analytic.hpp"
#include "geometry_sampled_field.hpp"
#include "model.hpp"
#include "utils.hpp"
#include <QFile>
#include <QImage>

using namespace sme;

SCENARIO("Analytic geometry", "[core/model/geometry_analytic][core/"
                              "model][core][model][geometry_analytic]") {
  GIVEN("SBML model with 2d analytic geometry") {
    model::Model s;
    QFile f(":/test/models/analytic_2d.xml");
    f.open(QIODevice::ReadOnly);
    s.importSBMLString(f.readAll().toStdString());
    REQUIRE(s.getGeometry().getIsValid() == true);
    REQUIRE(s.getGeometry().getHasImage() == true);
    const auto &img = s.getGeometry().getImage();
    REQUIRE(img.colorCount() == 3);
    REQUIRE(img.size() == QSize(200, 200));
    REQUIRE(img.pixel(100, 100) == utils::indexedColours()[0].rgb());
    REQUIRE(img.pixel(80, 80) == utils::indexedColours()[1].rgb());
    REQUIRE(img.pixel(30, 20) == utils::indexedColours()[2].rgb());
  }
  GIVEN("SBML model with 3d analytic geometry") {
    model::Model s;
    QFile f(":/test/models/analytic_3d.xml");
    f.open(QIODevice::ReadOnly);
    s.importSBMLString(f.readAll().toStdString());
    REQUIRE(s.getGeometry().getIsValid() == true);
    REQUIRE(s.getGeometry().getHasImage() == true);
    const auto &img = s.getGeometry().getImage();
    REQUIRE(img.colorCount() == 3);
    REQUIRE(img.size() == QSize(200, 200));
    REQUIRE(img.pixel(100, 100) == utils::indexedColours()[0].rgb());
    REQUIRE(img.pixel(80, 80) == utils::indexedColours()[1].rgb());
    REQUIRE(img.pixel(30, 20) == utils::indexedColours()[2].rgb());
  }
}

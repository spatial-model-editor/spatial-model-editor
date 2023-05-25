#include "catch_wrapper.hpp"
#include "sme/geometry_utils.hpp"

using namespace sme;

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

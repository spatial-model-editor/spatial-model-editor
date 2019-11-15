#include <QRgb>

#include "catch_wrapper.hpp"
#include "tiff.hpp"
#include "utils.hpp"

TEST_CASE("QStringList <-> std::vector<std::string>", "[utils]") {
  std::vector<std::string> s{"ab", "qwef", "Qvsdss!"};
  QStringList q;
  q << "ab";
  q << "qwef";
  q << "Qvsdss!";
  auto s2q = utils::toQString(s);
  auto q2s = utils::toStdString(q);
  REQUIRE(s2q == q);
  REQUIRE(q2s == s);
  REQUIRE(utils::toQString(q2s) == q);
  REQUIRE(utils::toStdString(s2q) == s);
}

TEST_CASE("vector <-> string: int", "[utils]") {
  std::vector<int> v{1, 4, 7, -12, 999, 22, 0, 0};
  std::string s("1 4 7 -12 999 22 0 0");
  auto v2s = utils::vectorToString(v);
  auto s2v = utils::stringToVector<int>(s);
  REQUIRE(s2v == v);
  REQUIRE(v2s == s);
  REQUIRE(utils::stringToVector<int>(v2s) == v);
}

TEST_CASE("vector <-> string: QRgba", "[utils]") {
  std::vector<QRgb> v{0xffffffff, 0x00ffffff, 123, 8435122, 0xfffffffe, 0};
  std::string s("4294967295 16777215 123 8435122 4294967294 0");
  auto v2s = utils::vectorToString(v);
  auto s2v = utils::stringToVector<QRgb>(s);
  REQUIRE(s2v == v);
  REQUIRE(v2s == s);
  REQUIRE(utils::stringToVector<QRgb>(v2s) == v);
}

TEST_CASE("vector <-> string: double", "[utils]") {
  std::vector<double> v{1.12341,     4.99,   1e-22, 999.123,
                        1e-11 + 2.1, 2.1001, -33,   88e22};
  auto v2s = utils::vectorToString(v);
  REQUIRE(utils::stringToVector<double>(v2s) == v);
}

TEST_CASE("QPointIndexer", "[utils]") {
  QSize size(20, 16);

  std::vector<QPoint> v{QPoint(1, 3), QPoint(5, 6), QPoint(9, 9)};
  utils::QPointIndexer qpi(size, v);

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

  qpi = utils::QPointIndexer(QSize(99, 99));
  REQUIRE(qpi.getIndex(v[0]).has_value() == false);
  qpi.addPoints(v);
  REQUIRE(qpi.getIndex(v[0]).has_value() == true);
}

TEST_CASE("QPointUniqueIndexer", "[utils]") {
  QSize size(20, 16);

  std::vector<QPoint> v{QPoint(1, 3), QPoint(5, 6), QPoint(9, 9)};
  utils::QPointUniqueIndexer qpi(size, v);
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

  qpi = utils::QPointUniqueIndexer(QSize(99, 99));
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

TEST_CASE("Read Tiff", "[utils][tiff]") {
  utils::TiffReader tiffReader("x");
  REQUIRE(tiffReader.size() == 0);
}

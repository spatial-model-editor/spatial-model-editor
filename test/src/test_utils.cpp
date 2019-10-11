#include <QRgb>

#include "catch.hpp"
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

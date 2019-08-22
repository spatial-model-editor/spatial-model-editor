#include "utils.hpp"

#include <QRgb>

#include "catch.hpp"

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

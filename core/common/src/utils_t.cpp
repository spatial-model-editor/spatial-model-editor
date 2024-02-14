#include "catch_wrapper.hpp"
#include "qt_test_utils.hpp"
#include "sme/tiff.hpp"
#include "sme/utils.hpp"
#include <QDir>
#include <QImage>
#include <QRgb>
#include <list>
#include <set>
#include <vector>

using namespace sme;

TEST_CASE("Utils", "[core/common/utils][core/common][core][utils]") {
  SECTION("sum/average of vector of ints") {
    std::vector<int> v{1, 2, 3, 4, 5, 6, -1};
    REQUIRE(common::sum(v) == 20);
    REQUIRE(common::average(v) == 20 / 7);
    REQUIRE(common::min(v) == -1);
    REQUIRE(common::max(v) == 6);
    auto [min, max] = common::minmax(v);
    REQUIRE(min == -1);
    REQUIRE(max == 6);
  }
  SECTION("sum/average of vector of doubles") {
    std::vector<double> v{1, 2, 3, 4, 5, 6, -1};
    REQUIRE(common::sum(v) == dbl_approx(20.0));
    REQUIRE(common::average(v) == dbl_approx(20.0 / 7.0));
    REQUIRE(common::min(v) == dbl_approx(-1.0));
    REQUIRE(common::max(v) == dbl_approx(6.0));
    auto [min, max] = common::minmax(v);
    REQUIRE(min == dbl_approx(-1.0));
    REQUIRE(max == dbl_approx(6.0));
  }
  SECTION("index of element in vector of doubles") {
    std::vector<double> v{1, 2, 3, 4, 5, 6, -1};
    REQUIRE(common::element_index(v, 1) == 0);
    REQUIRE(common::element_index(v, 2) == 1);
    REQUIRE(common::element_index(v, 3) == 2);
    REQUIRE(common::element_index(v, -1) == 6);
    // element not found: return 0 by default
    REQUIRE(common::element_index(v, -3) == 0);
    // element not found: specify return value for this
    REQUIRE(common::element_index(v, -3, 999) == 999);
  }
  SECTION("QStringList <-> std::vector<std::string>") {
    std::vector<std::string> s{"ab", "qwef", "Qvsdss!"};
    QStringList q;
    q << "ab";
    q << "qwef";
    q << "Qvsdss!";
    auto s2q = common::toQString(s);
    auto q2s = common::toStdString(q);
    REQUIRE(s2q == q);
    REQUIRE(q2s == s);
    REQUIRE(common::toQString(q2s) == q);
    REQUIRE(common::toStdString(s2q) == s);
    QStringList qEmpty;
    std::vector<std::string> sEmpty;
    REQUIRE(common::toStdString(qEmpty) == sEmpty);
    REQUIRE(common::toQString(sEmpty) == qEmpty);
  }
  SECTION("vector <-> string") {
    SECTION("type: int") {
      std::vector<int> v{1, 4, 7, -12, 999, 22, 0, 0};
      std::string s("1 4 7 -12 999 22 0 0");
      auto v2s = common::vectorToString(v);
      auto s2v = common::stringToVector<int>(s);
      REQUIRE(s2v == v);
      REQUIRE(v2s == s);
      REQUIRE(common::stringToVector<int>(v2s) == v);
    }
    SECTION("type: int, empty") {
      auto v2s = common::vectorToString(std::vector<int>{});
      auto s2v = common::stringToVector<int>("");
      REQUIRE(v2s == "");
      REQUIRE(s2v == std::vector<int>{});
    }
    SECTION("type: QRgba") {
      std::vector<QRgb> v{0xffffffff, 0x00ffffff, 123, 8435122, 0xfffffffe, 0};
      std::string s("4294967295 16777215 123 8435122 4294967294 0");
      auto v2s = common::vectorToString(v);
      auto s2v = common::stringToVector<QRgb>(s);
      REQUIRE(s2v == v);
      REQUIRE(v2s == s);
      REQUIRE(common::stringToVector<QRgb>(v2s) == v);
    }
    SECTION("type: double") {
      std::vector<double> v{1.12341,     4.99,   1e-22, 999.123,
                            1e-11 + 2.1, 2.1001, -33,   88e22};
      auto v2s = common::vectorToString(v);
      REQUIRE(common::stringToVector<double>(v2s) == v);
    }
  }
  SECTION("int <-> bool") {
    std::vector<bool> vb{true, true, false, true, false};
    std::vector<int> vi{1, 1, 0, 1, 0};
    auto vb2i = common::toInt(vb);
    auto vi2b = common::toBool(vi);
    REQUIRE(vb2i == vi);
    REQUIRE(vi2b == vb);
    REQUIRE(common::toBool(vb2i) == vb);
    REQUIRE(common::toInt(vi2b) == vi);
  }
  SECTION("isItIndexes") {
    SECTION("the container contains indexes from 0 to n") {
      std::vector<double> A = {1, 2, 3, 5, 5, 4, 5, 5, 5, 5, 5, 5, 1, 2, 0, 5};
      std::vector<double> A1 = {1, 2, 3, 5, 5, 4, 5, 5, 5, 5, 5, 5, 1, 2, 5, 5};
      std::size_t size1{6};
      std::size_t size2{5};

      REQUIRE(sme::common::isItIndexes(A, size1) == true);
      REQUIRE(sme::common::isItIndexes(A1, size1) == false);
      REQUIRE(sme::common::isItIndexes(A, size2) == false);
      REQUIRE(sme::common::isItIndexes(A1, size2) == false);
    }
    SECTION("get unique values from a container") {
      std::vector<double> in = {1, 2, 3, 5, 5, 4, 5, 5, 5, 5, 5, 5, 1, 2, 0, 5};
      std::vector<int> in1 = {1, 2, 3, 5, 5, 4, 5, 5, 5, 5, 5, 5, 1, 2, 5};
      std::vector<double> out = {0, 1, 2, 3, 4, 5};
      std::vector<int> out1 = {1, 2, 3, 4, 5};
      REQUIRE(sme::common::get_unique_values(in) == out);
      REQUIRE(sme::common::get_unique_values(in1) == out1);
    }
  }
}

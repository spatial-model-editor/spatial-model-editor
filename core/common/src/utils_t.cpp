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
  SECTION("QPointIndexer") {
    QSize size(20, 16);

    std::vector<QPoint> v{QPoint(1, 3), QPoint(5, 6), QPoint(9, 9)};
    common::QPointIndexer qpi(size, v);

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

    qpi = common::QPointIndexer(QSize(99, 99));
    REQUIRE(qpi.getIndex(v[0]).has_value() == false);
    qpi.addPoints(v);
    REQUIRE(qpi.getIndex(v[0]).has_value() == true);
  }
  SECTION("QPointUniqueIndexer") {
    QSize size(20, 16);
    std::vector<QPoint> v{QPoint(1, 3), QPoint(5, 6), QPoint(9, 9)};
    common::QPointUniqueIndexer qpi(size, v);
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

    qpi = common::QPointUniqueIndexer(QSize(99, 99));
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
  SECTION("toGrayscaleIntensityImage") {
    SECTION("all values identical and non-zero should be white") {
      QSize sz1(2, 2);
      auto img1{common::toGrayscaleIntensityImage(
          sz1, std::vector<double>{1.2, 1.2, 1.2, 1.2})};
      REQUIRE(img1.size() == sz1);
      REQUIRE(img1.pixel(0, 0) == qRgb(255, 255, 255));
      REQUIRE(img1.pixel(0, 1) == qRgb(255, 255, 255));
      REQUIRE(img1.pixel(1, 0) == qRgb(255, 255, 255));
      REQUIRE(img1.pixel(1, 1) == qRgb(255, 255, 255));
    }
    SECTION("all values zero should be black") {
      QSize sz1(2, 2);
      auto img1{common::toGrayscaleIntensityImage(
          sz1, std::vector<double>{0, 0, 0, 0})};
      REQUIRE(img1.size() == sz1);
      REQUIRE(img1.pixel(0, 0) == qRgb(0, 0, 0));
      REQUIRE(img1.pixel(0, 1) == qRgb(0, 0, 0));
      REQUIRE(img1.pixel(1, 0) == qRgb(0, 0, 0));
      REQUIRE(img1.pixel(1, 1) == qRgb(0, 0, 0));
    }
    SECTION("pixels start bottom-left, scan along x, end at top right") {
      QSize sz1(2, 2);
      auto img1{common::toGrayscaleIntensityImage(
          sz1, std::vector<double>{1.0, 2.0, 3.0, 4.0})};
      REQUIRE(img1.size() == sz1);
      REQUIRE(img1.pixel(0, 1) == qRgb(63, 63, 63));
      REQUIRE(img1.pixel(1, 1) == qRgb(127, 127, 127));
      REQUIRE(img1.pixel(0, 0) == qRgb(191, 191, 191));
      REQUIRE(img1.pixel(1, 0) == qRgb(255, 255, 255));
    }
    SECTION("invalid array size produces black image") {
      QSize sz1(11, 13);
      // array too small
      auto img1{common::toGrayscaleIntensityImage(sz1, std::vector<double>{})};
      REQUIRE(img1.size() == sz1);
      REQUIRE(img1.pixel(0, 0) == qRgb(0, 0, 0));
      QSize sz2(1, 1);
      // array too large
      auto img2{common::toGrayscaleIntensityImage(
          sz2, std::vector<double>{1.0, 2.0})};
      REQUIRE(img2.size() == sz2);
      REQUIRE(img2.pixel(0, 0) == qRgb(0, 0, 0));
    }
  }
}

#include <QDir>
#include <QImage>
#include <QRgb>
#include <list>
#include <set>
#include <vector>

#include "catch_wrapper.hpp"
#include "qt_test_utils.hpp"
#include "tiff.hpp"
#include "utils.hpp"

SCENARIO("Utils", "[core][utils]") {
  GIVEN("sum/average of vector of ints") {
    std::vector<int> v{1, 2, 3, 4, 5, 6, -1};
    REQUIRE(utils::sum(v) == 20);
    REQUIRE(utils::average(v) == 20 / 7);
    REQUIRE(utils::min(v) == -1);
    REQUIRE(utils::max(v) == 6);
    auto [min, max] = utils::minmax(v);
    REQUIRE(min == -1);
    REQUIRE(max == 6);
  }
  GIVEN("sum/average of vector of doubles") {
    std::vector<double> v{1, 2, 3, 4, 5, 6, -1};
    REQUIRE(utils::sum(v) == dbl_approx(20.0));
    REQUIRE(utils::average(v) == dbl_approx(20.0 / 7.0));
    REQUIRE(utils::min(v) == dbl_approx(-1.0));
    REQUIRE(utils::max(v) == dbl_approx(6.0));
    auto [min, max] = utils::minmax(v);
    REQUIRE(min == dbl_approx(-1.0));
    REQUIRE(max == dbl_approx(6.0));
  }
  GIVEN("QStringList <-> std::vector<std::string>") {
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

    QStringList qEmpty;
    std::vector<std::string> sEmpty;
    REQUIRE(utils::toStdString(qEmpty) == sEmpty);
    REQUIRE(utils::toQString(sEmpty) == qEmpty);
  }
  GIVEN("vector <-> string") {
    WHEN("type: int") {
      std::vector<int> v{1, 4, 7, -12, 999, 22, 0, 0};
      std::string s("1 4 7 -12 999 22 0 0");
      auto v2s = utils::vectorToString(v);
      auto s2v = utils::stringToVector<int>(s);
      REQUIRE(s2v == v);
      REQUIRE(v2s == s);
      REQUIRE(utils::stringToVector<int>(v2s) == v);
    }
    WHEN("type: QRgba") {
      std::vector<QRgb> v{0xffffffff, 0x00ffffff, 123, 8435122, 0xfffffffe, 0};
      std::string s("4294967295 16777215 123 8435122 4294967294 0");
      auto v2s = utils::vectorToString(v);
      auto s2v = utils::stringToVector<QRgb>(s);
      REQUIRE(s2v == v);
      REQUIRE(v2s == s);
      REQUIRE(utils::stringToVector<QRgb>(v2s) == v);
    }
    WHEN("type: double") {
      std::vector<double> v{1.12341,     4.99,   1e-22, 999.123,
                            1e-11 + 2.1, 2.1001, -33,   88e22};
      auto v2s = utils::vectorToString(v);
      REQUIRE(utils::stringToVector<double>(v2s) == v);
    }
  }
  GIVEN("QPointIndexer") {
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
  GIVEN("QPointUniqueIndexer") {
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
  GIVEN("SmallSet of int") {
    utils::SmallStackSet<int, 1> ss;
    REQUIRE(ss.max_size() == 1);
    REQUIRE(ss.size() == 0);
    ss.insert(12);
    REQUIRE(ss.size() == 1);
    REQUIRE(ss[0] == 12);
    REQUIRE(*ss.cbegin() == 12);
    REQUIRE(ss.contains(12) == true);
    REQUIRE(ss.contains(10) == false);
    // insert is a no-op once set reaches max_size
    ss.insert(10);
    REQUIRE(ss.size() == 1);
    REQUIRE(ss[0] == 12);
    REQUIRE(ss.contains(12) == true);
    REQUIRE(ss.contains(10) == false);
    // erase non-existent element is also a no-op
    ss.erase(3);
    REQUIRE(ss.size() == 1);
    REQUIRE(ss.contains(12) == true);
    ss.erase(12);
    REQUIRE(ss.size() == 0);
    REQUIRE(ss.contains(12) == false);
    // can initialise with single value
    ss = utils::SmallStackSet<int, 1>(3);
    REQUIRE(ss.size() == 1);
    REQUIRE(ss.contains(3) == true);
  }
  GIVEN("SmallSet of size_t") {
    utils::SmallStackSet<std::size_t, 16> ss({1, 2, 3, 4});
    REQUIRE(ss.max_size() == 16);
    REQUIRE(ss.size() == 4);
    REQUIRE(ss.contains(1) == true);
    REQUIRE(ss.contains(2) == true);
    REQUIRE(ss.contains(3) == true);
    REQUIRE(ss.contains(4) == true);
    REQUIRE(ss.contains(5) == false);
    REQUIRE(ss.contains_any_of(std::vector<int>{5, 7}) == false);
    REQUIRE(ss.contains_any_of(std::vector<int>{5, 7, 2}) == true);
    REQUIRE(ss.contains_any_of(std::vector<int>{1, 2, 3}) == true);
    REQUIRE(ss.contains_any_of(std::list<int>{5, 7}) == false);
    REQUIRE(ss.contains_any_of(std::list<int>{5, 7, 2}) == true);
    REQUIRE(ss.contains_any_of(std::list<int>{1, 2, 3}) == true);
    ss.erase(2);
    REQUIRE(ss.size() == 3);
    REQUIRE(ss.contains(1) == true);
    REQUIRE(ss.contains(2) == false);
    REQUIRE(ss.contains(3) == true);
    REQUIRE(ss.contains(4) == true);
    REQUIRE(ss.contains_any_of(std::set<std::size_t>{5, 7, 2}) == false);
    REQUIRE(ss.contains_any_of(std::set<std::size_t>{5, 7, 2, 1}) == true);
    ss.erase(1);
    REQUIRE(ss.size() == 2);
    ss.clear();
    REQUIRE(ss.size() == 0);
    REQUIRE(ss.contains(1) == false);
    REQUIRE(ss.contains_any_of(std::vector<int>{1, 2, 3}) == false);
  }
}

SCENARIO("TiffReader", "[core][utils][tiff]") {
  GIVEN("1 16bit grayscale tiff") {
    QFile::remove("tmp.tif");
    QFile::copy(":/test/16bit_gray.tif", "tmp.tif");
    utils::TiffReader tiffReader(QDir::currentPath().toStdString() +
                                 QDir::separator().toLatin1() + "tmp.tif");
    REQUIRE(tiffReader.size() == 1);
    REQUIRE(tiffReader.getErrorMessage().isEmpty());
    auto img = tiffReader.getImage();
    REQUIRE(img.size() == QSize(100, 100));
    REQUIRE(img.pixel(0, 0) == 0xff000000);
  }
  GIVEN("3x 8bit grayscale tiff") {
    QFile::remove("tmp.tif");
    QFile::copy(":/test/8bit_gray_3x.tif", "tmp.tif");
    utils::TiffReader tiffReader(QDir::currentPath().toStdString() +
                                 QDir::separator().toLatin1() + "tmp.tif");
    REQUIRE(tiffReader.size() == 3);
    REQUIRE(tiffReader.getErrorMessage().isEmpty());
    auto img = tiffReader.getImage(0);
    REQUIRE(img.size() == QSize(5, 5));
    REQUIRE(img.pixel(0, 0) == 0xff000000);
  }
}

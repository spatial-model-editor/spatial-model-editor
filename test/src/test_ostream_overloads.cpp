#include "catch.hpp"
#include "ostream_overloads.hpp"

TEST_CASE("QByteArray", "[ostream][non-gui]") {
  std::stringstream ss;
  QByteArray qba("abc");
  ss << qba;
  REQUIRE(ss.str() == "\"abc\"");
}

TEST_CASE("QLatin1String", "[ostream][non-gui]") {
  std::stringstream ss;
  QLatin1String qls("abQ!");
  ss << qls;
  REQUIRE(ss.str() == "\"abQ!\"");
}

TEST_CASE("QString", "[ostream][non-gui]") {
  std::stringstream ss;
  QString qs("Q~z");
  ss << qs;
  REQUIRE(ss.str() == "\"Q~z\"");
}

TEST_CASE("QPoint", "[ostream][non-gui]") {
  std::stringstream ss;
  QPoint p(23, 66);
  ss << p;
  REQUIRE(ss.str() == "(23,66)");
}

TEST_CASE("QPointF", "[ostream][non-gui]") {
  std::stringstream ss;
  QPointF p(0.231, -9.66);
  ss << p;
  REQUIRE(ss.str() == "(0.231,-9.66)");
}

TEST_CASE("QSize", "[ostream][non-gui]") {
  std::stringstream ss;
  QSize p(1024, 768);
  ss << p;
  REQUIRE(ss.str() == "1024x768");
}

TEST_CASE("QColor", "[ostream][non-gui]") {
  std::stringstream ss;
  QColor c(123, 22, 99);
  ss << c;
  REQUIRE(ss.str() == "ff7b1663");
}

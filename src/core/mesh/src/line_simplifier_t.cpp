#include "catch_wrapper.hpp"
#include "line_simplifier.hpp"
#include <QPoint>

SCENARIO("Simplify Lines",
         "[core/mesh/line_simplifier][core/mesh][core][line_simplifier]") {
  GIVEN("Invalid points") {
    std::vector<QPoint> points;
    // 0 points
    mesh::LineSimplifier ls(points, true);
    REQUIRE(ls.isValid() == false);
    ls = mesh::LineSimplifier(points, false);
    REQUIRE(ls.isValid() == false);
    // 1 point
    points.emplace_back(0, 0);
    ls = mesh::LineSimplifier(points, true);
    REQUIRE(ls.isValid() == false);
    ls = mesh::LineSimplifier(points, false);
    REQUIRE(ls.isValid() == false);
    // 2 point loop
    points.emplace_back(0, 1);
    ls = mesh::LineSimplifier(points, true);
    REQUIRE(ls.isValid() == false);
    // 2 point non-loop is valid however
    ls = mesh::LineSimplifier(points, false);
    REQUIRE(ls.isValid() == true);
    // 3 points
    points.emplace_back(1, 1);
    ls = mesh::LineSimplifier(points, true);
    REQUIRE(ls.isValid() == true);
    ls = mesh::LineSimplifier(points, false);
    REQUIRE(ls.isValid() == true);
  }
  GIVEN("Loop") {
    // 9 point loop with 2 degenerate points
    std::vector<QPoint> points;
    points.emplace_back(0, 0);
    points.emplace_back(1, 0); // degenerate horizontal
    points.emplace_back(2, 0);
    points.emplace_back(3, 1);
    points.emplace_back(2, 2); // degenerate diagonal
    points.emplace_back(1, 3);
    points.emplace_back(0, 3);
    points.emplace_back(-1, 2);
    points.emplace_back(-1, 1);

    // optimal approx requires 7 points
    std::vector<QPoint> l7{{0, 0}, {2, 0},  {3, 1}, {1, 3},
                           {0, 3}, {-1, 2}, {-1, 1}};
    std::vector<QPoint> l6{{0, 0}, {2, 0}, {3, 1}, {1, 3}, {-1, 2}, {-1, 1}};
    std::vector<QPoint> l5{{0, 0}, {2, 0}, {3, 1}, {1, 3}, {-1, 2}};
    std::vector<QPoint> l4{{0, 0}, {3, 1}, {1, 3}, {-1, 2}};
    std::vector<QPoint> l3{{0, 0}, {3, 1}, {1, 3}};

    mesh::LineSimplifier ls(points, true);
    REQUIRE(ls.maxPoints() == l7.size());

    std::vector<QPoint> line;
    line.reserve(7);
    WHEN("set n points") {
      // requesting n>maxPoints() is equivalent to n=maxPoints()
      ls.getSimplifiedLine(line, 847);
      REQUIRE(line.size() == l7.size());
      REQUIRE(line.size() == ls.maxPoints());
      REQUIRE(line == l7);

      // n=7 approx
      ls.getSimplifiedLine(line, 7);
      REQUIRE(line.size() == 7);
      REQUIRE(line == l7);

      // n=6 approx
      ls.getSimplifiedLine(line, 6);
      REQUIRE(line.size() == 6);
      REQUIRE(line == l6);

      // n=5 approx
      ls.getSimplifiedLine(line, 5);
      REQUIRE(line.size() == 5);
      REQUIRE(line == l5);

      // n=4 approx
      ls.getSimplifiedLine(line, 4);
      REQUIRE(line.size() == 4);
      REQUIRE(line == l4);

      // n=3 approx
      ls.getSimplifiedLine(line, 3);
      REQUIRE(line.size() == 3);
      REQUIRE(line == l3);

      // n=3 is minimum number of vertices for loop
      ls.getSimplifiedLine(line, 2);
      REQUIRE(line.size() == 3);
      REQUIRE(line == l3);
    }
    WHEN("set max allowed deviation") {
      // allow zero deviation -> maximum number of points
      ls.getSimplifiedLine(line, {0, 0});
      REQUIRE(line.size() == l7.size());
      REQUIRE(line.size() == ls.maxPoints());
      REQUIRE(line == l7);

      // allow infinite deviation -> minimum number of points
      ls.getSimplifiedLine(line, {std::numeric_limits<double>::max(),
                                  std::numeric_limits<double>::max()});
      REQUIRE(line.size() == 3);
      REQUIRE(line == l3);

      // allow total deviation of 0.5 pixels
      ls.getSimplifiedLine(line, {0.5, 0});
      REQUIRE(line.size() == 6);

      // allow average deviation of 0.1 pixels
      ls.getSimplifiedLine(line, {0, 0.1});
      REQUIRE(line.size() == 5);

      // allow total deviation of 3 pixels
      ls.getSimplifiedLine(line, {3.0, 0});
      REQUIRE(line.size() == 4);

      // allow average deviation of 1 pixel
      ls.getSimplifiedLine(line, {0, 1.0});
      REQUIRE(line.size() == 3);
    }
  }
  GIVEN("Non-loop") {
    // 8 point line with 1 degenerate point
    std::vector<QPoint> points;
    points.emplace_back(0, 0);
    points.emplace_back(1, 0); // degenerate horizontal
    points.emplace_back(2, 0);
    points.emplace_back(3, 1);
    points.emplace_back(4, 1);
    points.emplace_back(4, 2);
    points.emplace_back(5, 3);
    points.emplace_back(5, 4);

    // optimal approx requires 7 points
    std::vector<QPoint> l7{{0, 0}, {2, 0}, {3, 1}, {4, 1},
                           {4, 2}, {5, 3}, {5, 4}};
    std::vector<QPoint> l6{{0, 0}, {2, 0}, {4, 1}, {4, 2}, {5, 3}, {5, 4}};
    std::vector<QPoint> l5{{0, 0}, {2, 0}, {4, 1}, {5, 3}, {5, 4}};
    std::vector<QPoint> l4{{0, 0}, {2, 0}, {4, 1}, {5, 4}};
    std::vector<QPoint> l3{{0, 0}, {4, 1}, {5, 4}};
    std::vector<QPoint> l2{{0, 0}, {5, 4}};

    mesh::LineSimplifier ls(points, false);
    REQUIRE(ls.maxPoints() == l7.size());

    std::vector<QPoint> line;
    line.reserve(7);
    WHEN("set n points") {
      // requesting n>maxPoints() is equivalent to n=maxPoints()
      ls.getSimplifiedLine(line, 847);
      REQUIRE(line.size() == l7.size());
      REQUIRE(line.size() == ls.maxPoints());
      REQUIRE(line == l7);

      // n=7 approx
      ls.getSimplifiedLine(line, 7);
      REQUIRE(line.size() == 7);
      REQUIRE(line == l7);

      // n=6 approx
      ls.getSimplifiedLine(line, 6);
      REQUIRE(line.size() == 6);
      REQUIRE(line == l6);

      // n=5 approx
      ls.getSimplifiedLine(line, 5);
      REQUIRE(line.size() == 5);
      REQUIRE(line == l5);

      // n=4 approx
      ls.getSimplifiedLine(line, 4);
      REQUIRE(line.size() == 4);
      REQUIRE(line == l4);

      // n=3 approx
      ls.getSimplifiedLine(line, 3);
      REQUIRE(line.size() == 3);
      REQUIRE(line == l3);

      // n=2 approx (just first & last points of line)
      ls.getSimplifiedLine(line, 2);
      REQUIRE(line.size() == 2);
      REQUIRE(line == l2);

      // n=2 is minimum number of vertices for non-loop line
      ls.getSimplifiedLine(line, 1);
      REQUIRE(line.size() == 2);
      REQUIRE(line == l2);
    }
    WHEN("set max allowed deviation") {
      // allow error deviation -> max number of points
      ls.getSimplifiedLine(line, {0, 0});
      REQUIRE(line.size() == l7.size());
      REQUIRE(line.size() == ls.maxPoints());
      REQUIRE(line == l7);

      // allow infinite deviation -> minimum number of points
      ls.getSimplifiedLine(line, {std::numeric_limits<double>::max(),
                                  std::numeric_limits<double>::max()});
      REQUIRE(line.size() == 2);
      REQUIRE(line == l2);

      // allow total deviation of 0.5 pixels
      ls.getSimplifiedLine(line, {0.5, 0});
      REQUIRE(line.size() == 6);

      // allow average deviation of 0.1 pixels
      ls.getSimplifiedLine(line, {0, 0.1});
      REQUIRE(line.size() == 6);

      // allow total deviation of 1.5 pixels
      ls.getSimplifiedLine(line, {1.5, 0});
      REQUIRE(line.size() == 4);

      // allow average deviation of 0.25 pixel
      ls.getSimplifiedLine(line, {0, 0.25});
      REQUIRE(line.size() == 3);

      // allow total deviation of 3 pixels
      ls.getSimplifiedLine(line, {3.0, 0});
      REQUIRE(line.size() == 3);

      // allow average deviation of 1 pixel
      ls.getSimplifiedLine(line, {0, 1.0});
      REQUIRE(line.size() == 2);
    }
  }
}

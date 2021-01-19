// Line Simplification
//   - Visvalingam-Whyatt polyline simplification
//   - see https://www.tandfonline.com/doi/abs/10.1179/000870493786962263
//   - start with ordered set of points forming a line
//   - constructs an n-point approximation to the line
//   - can also choose n given a maximum desired relative or absolute error

#pragma once

#include <QPoint>
#include <cstddef>
#include <limits>
#include <vector>

namespace sme {

namespace mesh {

struct LineError {
  double total = std::numeric_limits<double>::max();
  double average = std::numeric_limits<double>::max();
};

class LineSimplifier {
private:
  std::vector<QPoint> vertices;
  std::size_t minNumPoints;
  std::vector<std::size_t> priorities;
  LineError getLineError(const std::vector<QPoint> &line) const;
  bool valid{true};
  bool closedLoop{false};

public:
  void getSimplifiedLine(std::vector<QPoint> &line,
                         const LineError &allowedError = {0.0, 0.4}) const;
  void getSimplifiedLine(std::vector<QPoint> &line, std::size_t nPoints) const;
  const std::vector<QPoint> &getAllVertices() const;
  std::size_t maxPoints() const;
  bool isValid() const;
  bool isLoop() const;
  explicit LineSimplifier(const std::vector<QPoint> &points,
                          bool isClosedLoop = false);
};

} // namespace mesh

} // namespace sme

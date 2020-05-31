// Line Simplification
//   - Visvalingam-Whyatt polyline simplification
//   - see https://www.tandfonline.com/doi/abs/10.1179/000870493786962263
//   - start with ordered set of points forming a line
//   - constructs an n-point approximation to line given n
//   - or chooses n given maximum desired relative or absolute error

#pragma once

#include <QPoint>
#include <vector>

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

public:
  void getSimplifiedLine(std::vector<QPoint> &line,
                         const LineError &allowedError = {1.0, 0.2}) const;
  void getSimplifiedLine(std::vector<QPoint> &line, std::size_t nPoints) const;
  std::size_t maxPoints() const;
  bool isValid() const;
  explicit LineSimplifier(const std::vector<QPoint> &points,
                          bool isClosedLoop = false);
};

} // namespace mesh

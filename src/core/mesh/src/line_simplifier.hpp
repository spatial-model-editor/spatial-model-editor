#pragma once

#include <QPoint>
#include <cstddef>
#include <limits>
#include <vector>

namespace sme::mesh {

struct LineError {
  double total = std::numeric_limits<double>::max();
  double average = std::numeric_limits<double>::max();
};

/**
 * @brief Simplify a line using Visvalingam-Whyatt polyline simplification
 *
 * Given an ordered set of pixels that form a line or a closed loop, construct
 * an n-point approximation to the line.
 *
 * @note Each point in the line is ranked according to the size of the triangle
 * formed by itself and its two nearest neighbours, and points are removed in
 * reverse order of importance. See
 * [10.1179/000870493786962263](https://www.tandfonline.com/doi/abs/10.1179/000870493786962263)
 * for more details.
 */
class LineSimplifier {
private:
  std::vector<QPoint> vertices;
  std::size_t minNumPoints;
  std::vector<std::size_t> priorities;
  [[nodiscard]] LineError getLineError(const std::vector<QPoint> &line) const;
  bool valid{true};
  bool closedLoop{false};

public:
  /**
   * @brief Construct a simplified line given a maximum allowed error
   *
   * @param[out] line the simplified lines
   * @param[in] allowedError the maximum allowed error
   */
  void getSimplifiedLine(std::vector<QPoint> &line,
                         const LineError &allowedError = {0.0, 0.4}) const;
  /**
   * @brief Construct a simplified line given a maximum number of points
   *
   * @param[out] line the simplified lines
   * @param[in] nPoints the maximum allowed number of points
   */
  void getSimplifiedLine(std::vector<QPoint> &line, std::size_t nPoints) const;
  /**
   * @brief The original un-simplified line
   */
  [[nodiscard]] const std::vector<QPoint> &getAllVertices() const;
  /**
   * @brief The number of points in the original un-simplified line
   */
  [[nodiscard]] std::size_t maxPoints() const;
  /**
   * @brief Is the line valid
   */
  [[nodiscard]] bool isValid() const;
  /**
   * @brief Is the line a closed loop
   */
  [[nodiscard]] bool isLoop() const;
  /**
   * @brief Construct a simplified version of the supplied line
   *
   * @param[in] points the ordered points that define the line
   * @param[in] isClosedLoop true if the line represents a closed loop, i.e. the
   *    last point implicitly connects to the first point
   */
  explicit LineSimplifier(const std::vector<QPoint> &points,
                          bool isClosedLoop = false);
};

} // namespace sme::mesh

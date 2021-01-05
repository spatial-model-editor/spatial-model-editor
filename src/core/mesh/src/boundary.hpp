// Boundary
//   - contains an ordered set of points representing the boundary
//   - number of points can be updated to increase/reduce resolution

#pragma once
#include "line_simplifier.hpp"
#include <opencv2/core/types.hpp>
#include <QImage>
#include <QPoint>
#include <QPointF>
#include <cstddef>
#include <limits>
#include <string>
#include <vector>

namespace mesh {

struct Contours {
  std::vector<std::vector<cv::Point>> compartmentEdges;
  std::vector<std::vector<cv::Point>> domainEdges;
};

class Boundary {
private:
  LineSimplifier lineSimplifier;
  std::size_t maxPoints{};
  std::vector<QPoint> points;

public:
  bool isLoop() const;
  bool isValid() const;
  const std::vector<QPoint> &getPoints() const;
  const std::vector<QPoint> &getAllPoints() const;
  std::size_t getMaxPoints() const;
  void setMaxPoints(std::size_t maxPoints);
  std::size_t setMaxPoints();
  explicit Boundary(const std::vector<QPoint> &boundaryPoints,
                    bool isClosedLoop = false);
};

std::vector<Boundary>
constructBoundaries(const QImage &img,
                    const std::vector<QRgb> &compartmentColours, std::vector<std::vector<QPointF>>* interiorPoints = nullptr);

} // namespace mesh

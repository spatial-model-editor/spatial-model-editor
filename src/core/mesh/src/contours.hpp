// Contours
//   - inputs:
//      - segmented image of geometry: one colour for each compartment
//      - list of compartment colours
//      - list of membrane colour pairs
//   - outputs:
//      - boundaries & fixed points

#pragma once

#include "boundary.hpp"
#include "mesh_types.hpp"
#include <QImage>
#include <QRgb>
#include <array>
#include <cstddef>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/mat.inl.hpp>
#include <opencv2/core/types.hpp>
#include <string>
#include <utility>
#include <vector>

namespace mesh {

class Contours {
private:
  std::vector<Boundary> boundaries;
  std::vector<FixedPoint> fixedPoints;
  QImage boundaryPixelsImage;

public:
  Contours(const QImage &img, const std::vector<QRgb> &compartmentColours,
           const std::vector<std::pair<std::string, ColourPair>>
               &membraneColourPairs);
  const QImage &getBoundaryPixelsImage() const;
  std::vector<FixedPoint> &getFixedPoints();
  std::vector<Boundary> &getBoundaries();
};

class ContourMap {
private:
  inline const static std::array<cv::Point, 4> nn4{
      cv::Point(1, 0), cv::Point(-1, 0), cv::Point(0, -1), cv::Point(0, 1)};
  cv::Mat indices;

public:
  ContourMap(const QSize &size,
             const std::vector<std::vector<cv::Point>> &contours);
  std::size_t getContourIndex(const cv::Point &p) const;
  std::size_t getAdjacentContourIndex(const cv::Point &p) const;
  bool hasNeighbourWithThisIndex(const cv::Point &p,
                                 std::size_t contourIndex) const;
  bool hasNeighbourWithThisIndex(const std::vector<cv::Point> &points,
                                 std::size_t contourIndex) const;
};

} // namespace mesh

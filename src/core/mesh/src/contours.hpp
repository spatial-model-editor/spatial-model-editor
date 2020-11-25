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
#include <optional>
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

} // namespace mesh

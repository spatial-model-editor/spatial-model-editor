// ImageBoundaries

#pragma once

#include <QImage>
#include <QPoint>
#include <array>
#include <vector>

#include "boundary.hpp"
#include "mesh_types.hpp"

namespace mesh {

class ImageBoundaries {
private:
  std::vector<Boundary> boundaries;
  std::vector<FixedPoint> fixedPoints;
  std::vector<QPointF> newFixedPoints;
  QImage boundaryPixelsImage;
  double getFpRadius(const FixedPoint &fp) const;
  void updateFpAngles(FixedPoint &fp);
  std::vector<double> getMidpointAngles(const FixedPoint &fp) const;
  void expandFP(FixedPoint &fp);
  void expandFPs();
  void constructLines(const QImage &img,
                      const std::vector<QRgb> &compartmentColours,
                      const std::vector<std::pair<std::string, ColourPair>>
                          &membraneColourPairs);

public:
  ImageBoundaries();
  explicit ImageBoundaries(const QImage &img,
                           const std::vector<QRgb> &compartmentColours = {},
                           const std::vector<std::pair<std::string, ColourPair>>
                               &membraneColourPairs = {});
  ~ImageBoundaries();
  const std::vector<Boundary> &getBoundaries() const;
  const std::vector<FixedPoint> &getFixedPoints() const;
  const std::vector<QPointF> &getNewFixedPoints() const;
  const QImage &getBoundaryPixelsImage() const;
  void setMaxPoints(const std::vector<std::size_t> &boundaryMaxPoints);
  std::vector<std::size_t> setAutoMaxPoints();
  void setMaxPoints(std::size_t boundaryIndex, std::size_t maxPoints = 12);
  void setMembraneWidths(const std::vector<double> &newMembraneWidths);
  void setMembraneWidth(std::size_t boundaryIndex, double newMembraneWidth);
};

} // namespace mesh

// Boundary
//   - contains an ordered set of points representing the boundary
//   - number of points can be updated to increase/reduce resolution
//   - if isMembrane, then provides a pair of boundaries separated by
//   membraneWidth

#pragma once
#include <QImage>
#include <QPointF>
#include <array>
#include <vector>

#include "line_simplifier.hpp"
#include "mesh_types.hpp"

namespace mesh {

class Boundary {
private:
  static constexpr std::size_t nullIndex =
      std::numeric_limits<std::size_t>::max();
  bool loop;
  bool membrane;
  bool valid;
  std::string membraneID;
  LineSimplifier lineSimplifier;
  std::size_t maxPoints{};
  // approx to boundary lines using at most maxPoints
  std::vector<QPoint> points;
  std::vector<QPointF> innerPoints;
  std::vector<QPointF> outerPoints;
  // indices of fixed points shared by multiple boundaries
  FpIndices fpIndices = {nullIndex, nullIndex};
  FpIndices innerFpIndices = {nullIndex, nullIndex};
  FpIndices outerFpIndices = {nullIndex, nullIndex};
  double membraneWidth = 1;
  void constructMembraneBoundarySegment(std::size_t index,
                                        const QPointF &normal);
  void constructMembraneBoundaries();

public:
  bool isLoop() const;
  bool isMembrane() const;
  bool isValid() const;
  const std::vector<QPoint> &getPoints() const;
  const std::vector<QPointF> &getInnerPoints() const;
  void setInnerStartPoint(const QPointF &point, std::size_t index);
  void setInnerEndPoint(const QPointF &point, std::size_t index);
  const std::vector<QPointF> &getOuterPoints() const;
  void setOuterStartPoint(const QPointF &point, std::size_t index);
  void setOuterEndPoint(const QPointF &point, std::size_t index);
  const FpIndices &getFpIndices() const;
  void setFpIndices(const FpIndices &indices);
  const FpIndices &getInnerFpIndices() const;
  const FpIndices &getOuterFpIndices() const;
  std::size_t getMaxPoints() const;
  void setMaxPoints(std::size_t maxPoints);
  std::size_t setMaxPoints();
  double getMembraneWidth() const;
  void setMembraneWidth(double newMembraneWidth);
  const std::string &getMembraneId() const;
  explicit Boundary(const std::vector<QPoint> &boundaryPoints,
                    bool isClosedLoop = false,
                    bool isMembraneCompartment = false,
                    std::string membraneName = {});
};

} // namespace mesh

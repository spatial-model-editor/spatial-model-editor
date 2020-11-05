// Boundary
//   - contains an ordered set of points representing the boundary
//   - number of points can be updated to increase/reduce resolution

#pragma once
#include "line_simplifier.hpp"
#include "mesh_types.hpp"
#include <QPoint>
#include <QPointF>
#include <cstddef>
#include <limits>
#include <string>
#include <vector>

namespace mesh {

class Boundary {
private:
  static constexpr std::size_t nullIndex =
      std::numeric_limits<std::size_t>::max();
  bool loop;
  bool membrane;
  bool valid;
  std::string membraneID;
  std::size_t membraneIndex;
  LineSimplifier lineSimplifier;
  std::size_t maxPoints{};
  // approx to boundary lines using at most maxPoints
  std::vector<QPoint> points;
  // indices of fixed points shared by multiple boundaries
  FpIndices fpIndices = {nullIndex, nullIndex};

public:
  bool isLoop() const;
  bool isMembrane() const;
  bool isValid() const;
  const std::vector<QPoint> &getPoints() const;
  const FpIndices &getFpIndices() const;
  void setFpIndices(const FpIndices &indices);
  std::size_t getMaxPoints() const;
  void setMaxPoints(std::size_t maxPoints);
  std::size_t setMaxPoints();
  const std::string &getMembraneId() const;
  std::size_t getMembraneIndex() const;
  explicit Boundary(const std::vector<QPoint> &boundaryPoints,
                    bool isClosedLoop = false,
                    bool isMembraneCompartment = false,
                    const std::string &membraneName = {},
                    std::size_t membraneIndex = 0);
};

} // namespace mesh

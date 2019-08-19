#include "mesh.hpp"

#include <set>

#include <QDebug>
#include <QPainter>

#include "colours.hpp"
#include "logger.hpp"

static int qPointToInt(const QPoint& point) {
  return point.x() + 65536 * point.y();
}

namespace mesh {

bool BoundaryBoolGrid::isBoundary(std::size_t x, std::size_t y) const {
  return grid[x + 1][y + 1];
}

bool BoundaryBoolGrid::isBoundary(const QPoint& point) const {
  auto x = static_cast<std::size_t>(point.x());
  auto y = static_cast<std::size_t>(point.y());
  return isBoundary(x, y);
}

bool BoundaryBoolGrid::isFixed(std::size_t x, std::size_t y) const {
  return fixedPointIndex[x + 1][y + 1] != NULL_INDEX;
}

bool BoundaryBoolGrid::isFixed(const QPoint& point) const {
  auto x = static_cast<std::size_t>(point.x());
  auto y = static_cast<std::size_t>(point.y());
  return isFixed(x, y);
}

std::size_t BoundaryBoolGrid::getFixedPointIndex(std::size_t x,
                                                 std::size_t y) const {
  return fixedPointIndex[x + 1][y + 1];
}

std::size_t BoundaryBoolGrid::getFixedPointIndex(const QPoint& point) const {
  auto x = static_cast<std::size_t>(point.x());
  auto y = static_cast<std::size_t>(point.y());
  return getFixedPointIndex(x, y);
}

const QPoint& BoundaryBoolGrid::getFixedPoint(const QPoint& point) const {
  auto x = static_cast<std::size_t>(point.x());
  auto y = static_cast<std::size_t>(point.y());
  return fixedPoints.at(getFixedPointIndex(x, y));
}

void BoundaryBoolGrid::setBoundaryPoint(const QPoint& point, bool multi) {
  auto x = static_cast<std::size_t>(point.x() + 1);
  auto y = static_cast<std::size_t>(point.y() + 1);
  if (multi) {
    std::size_t i;
    if (isFixed(point)) {
      i = getFixedPointIndex(point);
      spdlog::debug(
          "BoundaryBoolGrid::setBoundaryPoint :: adding {} to existing FP {}",
          point, fixedPoints.at(i));
    } else {
      spdlog::debug("BoundaryBoolGrid::setBoundaryPoint :: creating new FP {}",
                    point);
      fixedPoints.push_back(point);
      fixedPointCounter.push_back(3);
      i = fixedPoints.size() - 1;
    }
    // set all neighbouring pixels to point to this fixed point
    fixedPointIndex[x][y] = i;
    fixedPointIndex[x + 1][y] = i;
    fixedPointIndex[x - 1][y] = i;
    fixedPointIndex[x][y + 1] = i;
    fixedPointIndex[x][y - 1] = i;
    fixedPointIndex[x + 1][y + 1] = i;
    fixedPointIndex[x + 1][y - 1] = i;
    fixedPointIndex[x - 1][y + 1] = i;
    fixedPointIndex[x - 1][y - 1] = i;
  }
  spdlog::debug("BoundaryBoolGrid::setBoundaryPoint :: BP: {}", point);
  grid[x][y] = true;
}

void BoundaryBoolGrid::visitPoint(std::size_t x, std::size_t y) {
  grid[x + 1][y + 1] = false;
}

void BoundaryBoolGrid::visitPoint(const QPoint& point) {
  auto x = static_cast<std::size_t>(point.x());
  auto y = static_cast<std::size_t>(point.y());
  visitPoint(x, y);
}

void BoundaryBoolGrid::unvisitPoint(std::size_t x, std::size_t y) {
  grid[x + 1][y + 1] = true;
}

void BoundaryBoolGrid::unvisitPoint(const QPoint& point) {
  auto x = static_cast<std::size_t>(point.x());
  auto y = static_cast<std::size_t>(point.y());
  unvisitPoint(x, y);
}

BoundaryBoolGrid::BoundaryBoolGrid(const QImage& inputImage)
    : grid(static_cast<size_t>(inputImage.width() + 2),
           std::vector<bool>(static_cast<size_t>(inputImage.height() + 2),
                             false)),
      fixedPointIndex(
          static_cast<size_t>(inputImage.width() + 2),
          std::vector<std::size_t>(static_cast<size_t>(inputImage.height() + 2),
                                   NULL_INDEX)) {
  auto img = inputImage.convertToFormat(QImage::Format_Indexed8);
  spdlog::debug("BoundaryBoolGrid::BoundaryBoolGrid :: nColours = {}",
                img.colorCount());
  // check colours of 4 nearest neighbours of each pixel
  for (int x = 0; x < img.width(); ++x) {
    for (int y = 0; y < img.height(); ++y) {
      QPoint point(x, y);
      int colIndex = img.pixelIndex(point);
      std::set<int> colours;
      colours.insert(colIndex);
      for (const auto& dp :
           {QPoint(1, 0), QPoint(-1, 0), QPoint(0, 1), QPoint(0, -1)}) {
        QPoint nn = point + dp;
        if (img.valid(nn)) {
          colours.insert(img.pixelIndex(nn));
        } else {
          // assume external pixels have same colour index as top-left pixel
          colours.insert(img.pixelIndex(0, 0));
        }
      }
      int largestColIndex = *colours.rbegin();
      std::size_t neighbours = colours.size() - 1;
      // QImage has (0,0) in top left
      // we want to generate mesh with (0,0) in bottom left
      // so here we invert the y value
      QPoint bottomLeftIndexedPoint = QPoint(x, img.height() - 1 - y);
      if (neighbours == 1 && largestColIndex == colIndex) {
        // - point has one other colour as neighbour
        // - this colour index is the larger
        setBoundaryPoint(bottomLeftIndexedPoint);
      } else if (neighbours > 1) {
        // - point has multiple colour neighbours
        setBoundaryPoint(bottomLeftIndexedPoint, true);
      }
    }
  }
}

static double triangleArea(const QPoint& a, const QPoint& b, const QPoint& c) {
  // https://en.wikipedia.org/wiki/Shoelace_formula
  return 0.5 * std::fabs(a.x() * b.y() + b.x() * c.y() + c.x() * a.y() -
                         b.x() * a.y() - c.x() * b.y() - a.x() * c.y());
}

Mesh::Mesh(const QImage& image, const std::vector<QPointF>& interiorPoints,
           const std::vector<std::size_t>& maxPoints,
           const std::vector<std::size_t>& maxTriangleArea)
    : img(image),
      compartmentInteriorPoints(interiorPoints),
      boundaryMaxPoints(maxPoints),
      compartmentMaxTriangleArea(maxTriangleArea) {
  constructFullBoundaries();
  spdlog::info("Mesh::Mesh :: found {} boundaries", fullBoundaries.size());
  for (const auto& boundary : fullBoundaries) {
    spdlog::info("Mesh::Mesh ::   - {} points, loop={}", boundary.points.size(),
                 boundary.isLoop);
  }
  boundaries = fullBoundaries;
  if (boundaryMaxPoints.empty()) {
    // if boundary points not specified use default value
    boundaryMaxPoints = std::vector<std::size_t>(fullBoundaries.size(),
                                                 defaultBoundaryMaxPoints);
    spdlog::info(
        "Mesh::Mesh :: no boundaryMaxPoints values specified, using default "
        "value: {}",
        defaultBoundaryMaxPoints);
  }
  updateBoundaries();
  spdlog::info("Mesh::Mesh :: simplified {} boundaries", boundaries.size());
  for (const auto& boundary : boundaries) {
    spdlog::info("Mesh::Mesh ::   - {} points, loop={}", boundary.points.size(),
                 boundary.isLoop);
  }
  if (compartmentMaxTriangleArea.empty()) {
    // if triangle areas not specified use default value
    compartmentMaxTriangleArea = std::vector<std::size_t>(
        interiorPoints.size(), defaultCompartmentMaxTriangleArea);
    spdlog::info(
        "Mesh::Mesh :: no max triangle areas specified, using default value: "
        "{}",
        defaultCompartmentMaxTriangleArea);
  }
  // todo: holes?
  constructMesh();
}

void Mesh::setBoundaryMaxPoints(std::size_t boundaryIndex,
                                std::size_t maxPoints) {
  spdlog::info(
      "Mesh::setBoundaryMaxPoint :: boundaryIndex {}: max points {} -> {}",
      boundaryIndex, boundaryMaxPoints.at(boundaryIndex), maxPoints);
  boundaryMaxPoints.at(boundaryIndex) = maxPoints;
  updateBoundary(boundaryIndex);
}

std::size_t Mesh::getBoundaryMaxPoints(std::size_t boundaryIndex) const {
  return boundaryMaxPoints.at(boundaryIndex);
}

void Mesh::setCompartmentMaxTriangleArea(std::size_t compartmentIndex,
                                         std::size_t maxTriangleArea) {
  spdlog::info(
      "Mesh::setCompartmentMaxTriangleArea :: compIndex {}: max triangle area "
      "{} -> {}",
      compartmentIndex, compartmentMaxTriangleArea.at(compartmentIndex),
      maxTriangleArea);
  compartmentMaxTriangleArea.at(compartmentIndex) = maxTriangleArea;
  constructMesh();
}

std::size_t Mesh::getCompartmentMaxTriangleArea(
    std::size_t compartmentIndex) const {
  return compartmentMaxTriangleArea.at(compartmentIndex);
}

// in-place Visvalingam polyline simplification
// NB: very inefficient & not quite right:
// when recalculating triangle, if new one is smaller
// than the one just removed, should use the just-removed value
// for the area - but good enough for now
void Mesh::simplifyBoundary(Boundary& bp, std::size_t maxPoints) const {
  std::size_t size = bp.points.size();
  double minArea = std::numeric_limits<double>::max();
  double totalArea = 0;
  std::size_t iMin = 0;
  std::size_t iMax = size;
  auto iter = bp.points.begin();
  // if boundary is not a loop, then first and last points are fixed
  if (!bp.isLoop) {
    ++iMin;
    ++iter;
    --iMax;
  }
  // calculate area of triangle for each point
  auto iterSmallest = iter;
  for (std::size_t i = iMin; i < iMax; ++i) {
    std::size_t ip = (i + 1) % size;
    std::size_t im = (i + size - 1) % size;
    double area = triangleArea(bp.points[im], bp.points[i], bp.points[ip]);
    if (area < minArea) {
      minArea = area;
      iterSmallest = iter;
    }
    totalArea += area;
    ++iter;
  }
  if (size < 4 || (minArea > 0 && size <= maxPoints)) {
    // if we have less than 4 points,
    // or less than maxPoints and the minArea is non-zero,
    // then we are done
    return;
  }
  // remove point with smallest triangle
  bp.points.erase(iterSmallest);
  // repeat
  simplifyBoundary(bp, maxPoints);
}

void Mesh::updateBoundary(std::size_t boundaryIndex) {
  // reset boundary to full un-simplified one
  boundaries.at(boundaryIndex) = fullBoundaries.at(boundaryIndex);
  // simplify it
  simplifyBoundary(boundaries.at(boundaryIndex),
                   boundaryMaxPoints.at(boundaryIndex));
}

void Mesh::updateBoundaries() {
  for (std::size_t i = 0; i < boundaries.size(); ++i) {
    updateBoundary(i);
  }
}

void Mesh::constructFullBoundaries() {
  fullBoundaries.clear();

  // add image boundary loop
  fullBoundaries.emplace_back();
  fullBoundaries.back().points.push_back(QPoint(0, 0));
  fullBoundaries.back().points.push_back(QPoint(0, img.height() - 1));
  fullBoundaries.back().points.push_back(
      QPoint(img.width() - 1, img.height() - 1));
  fullBoundaries.back().points.push_back(QPoint(img.width() - 1, 0));
  fullBoundaries.back().isLoop = true;

  // construct bool grid of all boundary points
  BoundaryBoolGrid bbg(img);

  // we now have an unordered set of all boundary points
  // with points used by multiple boundary lines identified as "fixed"
  //
  // two possible kinds of boundary
  //   - line between two fixed point
  //   - closed loop not involving any fixed points

  // do line between fixed points first:
  //   - start at a fixed point
  //   - visit nearest (unvisited) neighbouring boundary point in x or y
  //   - if not found, check diagonal neighbours
  //   - repeat until we hit another fixed point
  for (std::size_t i = 0; i < bbg.fixedPoints.size(); ++i) {
    const auto& fp = bbg.fixedPoints[i];
    bbg.visitPoint(fp);
    for (const auto& dfp : nearestNeighbourDirectionPoints) {
      if (bbg.isBoundary(fp + dfp) && bbg.fixedPointCounter.at(i) > 0) {
        Boundary boundary;
        boundary.points.push_back(fp);
        QPoint currPoint = fp + dfp;
        while (!bbg.isFixed(currPoint) ||
               (bbg.getFixedPoint(currPoint) == fp)) {
          // repeat until we hit another fixed point
          if (!(bbg.isFixed(currPoint) &&
                (bbg.getFixedPoint(currPoint) == fp))) {
            // only add point to boundary if
            // we are outside of the initial fixed point radius
            boundary.points.push_back(currPoint);
          }
          bbg.visitPoint(currPoint);
          for (const auto& directionPoint : nearestNeighbourDirectionPoints) {
            if (bbg.isBoundary(currPoint + directionPoint)) {
              currPoint += directionPoint;
              break;
            }
          }
        }
        // add final fixed point to boundary line
        boundary.points.push_back(bbg.getFixedPoint(currPoint));
        // we now have a line between two fixed points
        // remove any degenerate points to give the fullBoundary
        simplifyBoundary(boundary, std::numeric_limits<std::size_t>::max());
        fullBoundaries.push_back(boundary);
        --bbg.fixedPointCounter[i];
        --bbg.fixedPointCounter[bbg.getFixedPointIndex(currPoint)];
      }
    }
  }
  // any remaining points should be independent closed loops:
  //   - find (any non-fixed) boundary point
  //   - visit nearest (unvisited) neighbouring boundary point in x or y
  //   - if not found, check diagonal neighbours
  //   - if not found, loop is done
  bool foundStartPoint;
  do {
    // find a boundary point to start from
    QPoint startPoint;
    foundStartPoint = false;
    for (int x = 0; x < img.width(); ++x) {
      for (int y = 0; y < img.height(); ++y) {
        startPoint = QPoint(x, y);
        if (bbg.isBoundary(startPoint) && !bbg.isFixed(startPoint)) {
          foundStartPoint = true;
          break;
        }
      }
      if (foundStartPoint) {
        break;
      }
    }
    if (foundStartPoint) {
      Boundary boundary;
      boundary.isLoop = true;
      QPoint currPoint = startPoint;
      bool finished = false;
      while (!finished) {
        boundary.points.push_back(currPoint);
        finished = true;
        bbg.visitPoint(currPoint);
        for (const auto& directionPoint : nearestNeighbourDirectionPoints) {
          if (bbg.isBoundary(currPoint + directionPoint) &&
              !bbg.isFixed(currPoint + directionPoint)) {
            currPoint += directionPoint;
            finished = false;
            break;
          }
        }
      }
      // remove any degenerate points
      simplifyBoundary(boundary, std::numeric_limits<std::size_t>::max());
      fullBoundaries.push_back(boundary);
    }
  } while (foundStartPoint);
}

void Mesh::constructMesh() {
  // points may be used by multiple boundary lines,
  // so first construct a set of unique points with map to their index
  std::map<int, std::size_t> mapPointToIndex;
  std::size_t index = 0;
  std::vector<QPoint> boundaryPoints;
  for (const auto& boundary : boundaries) {
    for (const auto& point : boundary.points) {
      if (mapPointToIndex.find(qPointToInt(point)) == mapPointToIndex.cend()) {
        // QPoint not already in list: add it
        boundaryPoints.push_back(point);
        mapPointToIndex[qPointToInt(point)] = index;
        ++index;
      }
    }
  }
  // for each boundary line, replace each QPoint
  // with its index in the list of boundaryPoints
  std::vector<triangle_wrapper::BoundarySegments> boundarySegmentsVector;
  for (std::size_t i = 0; i < boundaries.size(); ++i) {
    boundarySegmentsVector.emplace_back();
    for (std::size_t j = 0; j < boundaries[i].points.size() - 1; ++j) {
      auto i0 = mapPointToIndex.at(qPointToInt(boundaries[i].points[j]));
      auto i1 = mapPointToIndex.at(qPointToInt(boundaries[i].points[j + 1]));
      boundarySegmentsVector.back().push_back({i0, i1});
    }
    if (boundaries[i].isLoop) {
      // connect last point to first point
      auto i0 = mapPointToIndex.at(qPointToInt(boundaries[i].points.back()));
      auto i1 = mapPointToIndex.at(qPointToInt(boundaries[i].points.front()));
      boundarySegmentsVector.back().push_back({i0, i1});
    }
  }
  // interior point & max triangle area for each compartment
  std::vector<triangle_wrapper::Compartment> compartments;
  for (std::size_t i = 0; i < compartmentInteriorPoints.size(); ++i) {
    compartments.emplace_back(compartmentInteriorPoints[i],
                              compartmentMaxTriangleArea[i]);
  }
  // generate mesh
  triangle_wrapper::Triangulate triangulate(
      boundaryPoints, boundarySegmentsVector, compartments, {});
  vertices = triangulate.getPoints();
  triangleIDs = triangulate.getTriangleIndices();
  spdlog::info("Mesh::constructMesh :: {} vertices, {} triangles",
               vertices.size(), triangleIDs.size());

  // construct triangles from triangle indices & vertices
  triangles = std::vector<std::vector<QTriangleF>>(compartments.size(),
                                                   std::vector<QTriangleF>{});
  for (const auto& t : triangleIDs) {
    triangles[t[0]].push_back({vertices[t[1]], vertices[t[2]], vertices[t[3]]});
  }
}

static double getScaleFactor(const QImage& img, const QSize& size) {
  double scaleFactor = 1;
  if (img.width() * size.height() > img.height() * size.width()) {
    scaleFactor = static_cast<double>(size.width() - 2) /
                  static_cast<double>(img.width());
  } else {
    scaleFactor = static_cast<double>(size.height() - 2) /
                  static_cast<double>(img.height());
  }
  return scaleFactor;
}

QImage Mesh::getBoundariesImage(const QSize& size,
                                std::size_t boldBoundaryIndex) const {
  double scaleFactor = getScaleFactor(img, size);
  // construct boundary image
  QImage boundaryImage(static_cast<int>(scaleFactor * img.width()),
                       static_cast<int>(scaleFactor * img.height()),
                       QImage::Format_ARGB32);
  boundaryImage.fill(QColor(0, 0, 0, 0));

  QPainter p(&boundaryImage);
  // draw boundary lines
  for (std::size_t k = 0; k < boundaries.size(); ++k) {
    const auto& boundary = boundaries[k];
    std::size_t maxPoint = boundary.points.size();
    if (!boundary.isLoop) {
      --maxPoint;
    }
    int penSize = 2;
    if (k == boldBoundaryIndex) {
      penSize = 5;
    }
    p.setPen(QPen(colours::indexedColours()[k], penSize));
    for (std::size_t i = 0; i < maxPoint; ++i) {
      p.drawEllipse(boundary.points[i] * scaleFactor, penSize, penSize);
      p.drawLine(
          boundary.points[i] * scaleFactor,
          boundary.points[(i + 1) % boundary.points.size()] * scaleFactor);
    }
  }
  p.end();
  // flip image on y-axis, to change (0,0) from bottom-left to top-left corner
  return boundaryImage.mirrored(false, true);
}

QImage Mesh::getMeshImage(const QSize& size,
                          std::size_t compartmentIndex) const {
  double scaleFactor = getScaleFactor(img, size);
  // construct mesh image
  QImage meshImage(static_cast<int>(scaleFactor * img.width()),
                   static_cast<int>(scaleFactor * img.height()),
                   QImage::Format_ARGB32);
  meshImage.fill(QColor(0, 0, 0, 0));
  QPainter p(&meshImage);
  // draw vertices
  p.setPen(QPen(Qt::red, 2));
  for (const auto& v : vertices) {
    p.drawEllipse(v * scaleFactor, 2, 2);
  }
  // fill triangles
  for (const auto& t : triangles[compartmentIndex]) {
    QPainterPath path;
    path.moveTo(t.back() * scaleFactor);
    for (const auto& tp : t) {
      path.lineTo(tp * scaleFactor);
    }
    p.fillPath(path, QBrush(QColor(235, 235, 255)));
  }
  // draw triangle lines
  for (std::size_t k = 0; k < triangles.size(); ++k) {
    p.setPen(QPen(Qt::gray, 1, Qt::DotLine));
    if (k == compartmentIndex) {
      p.setPen(QPen(Qt::black, 3, Qt::SolidLine));
    }
    for (const auto& t : triangles[k]) {
      p.drawLine(t[0] * scaleFactor, t[1] * scaleFactor);
      p.drawLine(t[1] * scaleFactor, t[2] * scaleFactor);
      p.drawLine(t[2] * scaleFactor, t[0] * scaleFactor);
    }
  }
  p.end();

  return meshImage.mirrored(false, true);
}

QString Mesh::getGMSH(double pixelPhysicalSize) const {
  // note: gmsh indexing starts with 1, so need to add 1 to all indices
  // note: gmsh (0,0) is bottom left, but in Qt it is top left, so flip y
  // todo: use actual xy values in physical units instead of pixels
  QString msh;
  msh.append("$MeshFormat\n");
  msh.append("2.2 0 8\n");
  msh.append("$EndMeshFormat\n");
  msh.append("$Nodes\n");
  msh.append(QString("%1\n").arg(vertices.size()));
  for (std::size_t i = 0; i < vertices.size(); ++i) {
    msh.append(QString("%1 %2 %3 %4\n")
                   .arg(i + 1)
                   .arg(vertices[i].x() * pixelPhysicalSize)
                   .arg(vertices[i].y() * pixelPhysicalSize)
                   .arg(0));
  }
  msh.append("$EndNodes\n");
  msh.append("$Elements\n");
  msh.append(QString("%1\n").arg(triangleIDs.size()));
  // order triangles by compartment index
  std::size_t maxCompIndex = 0;
  for (const auto& t : triangleIDs) {
    maxCompIndex = std::max(t[0], maxCompIndex);
  }
  std::size_t triangleIndex = 1;
  for (std::size_t compIndex = 0; compIndex <= maxCompIndex; ++compIndex) {
    for (const auto& t : triangleIDs) {
      if (t[0] == compIndex) {
        msh.append(QString("%1 2 2 %2 %2 %3 %4 %5\n")
                       .arg(triangleIndex)
                       .arg(t[0] + 1)
                       .arg(t[1] + 1)
                       .arg(t[2] + 1)
                       .arg(t[3] + 1));
        ++triangleIndex;
      }
    }
  }
  msh.append("$EndElements\n");
  return msh;
}

}  // namespace mesh

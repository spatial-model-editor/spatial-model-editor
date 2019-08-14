#include "mesh.hpp"

#include <set>

#include <QDebug>

#include "triangle/triangle.hpp"

#include "logger.hpp"

static int qPointToInt(const QPoint& point) {
  return point.x() + 65536 * point.y();
}

static void setPointList(struct triangle::triangulateio& s,
                         const std::vector<mesh::Boundary>& boundaries,
                         bool setSegments = false) {
  // points may be used by multiple boundary lines,
  // so first construct set of unique points for each line
  // with a mapping from each point to its index
  std::map<int, int> mapPointToIndex;
  int index = 0;
  std::size_t nPoints = 0;
  std::vector<std::vector<QPoint>> uniquePoints;
  for (const auto& boundary : boundaries) {
    std::vector<QPoint> unique;
    for (const auto& point : boundary.points) {
      if (mapPointToIndex.find(qPointToInt(point)) == mapPointToIndex.cend()) {
        // QPoint not already in list: add it
        unique.push_back(point);
        mapPointToIndex[qPointToInt(point)] = index;
        ++index;
      }
    }
    uniquePoints.push_back(unique);
    nPoints += unique.size();
  }

  s.pointlist = static_cast<double*>(malloc(2 * nPoints * sizeof(double)));
  s.numberofpoints = static_cast<int>(nPoints);
  s.pointmarkerlist = static_cast<int*>(malloc(nPoints * sizeof(int)));
  double* d = s.pointlist;
  int* m = s.pointmarkerlist;
  for (std::size_t i = 0; i < uniquePoints.size(); ++i) {
    for (const auto& p : uniquePoints[i]) {
      *d = static_cast<double>(p.x());
      ++d;
      *d = static_cast<double>(p.y());
      ++d;
      // marker id starts at 2, since 0 means not assigned, and 1 may be
      // assigned by the Triangle library
      *m = static_cast<int>(i) + 2;
      ++m;
    }
  }

  if (setSegments) {
    std::size_t nSegments = 0;
    for (const auto& boundary : boundaries) {
      nSegments += boundary.points.size();
      if (!boundary.isLoop) {
        --nSegments;
      }
    }
    s.segmentlist = static_cast<int*>(malloc(2 * nSegments * sizeof(int)));
    s.numberofsegments = static_cast<int>(nSegments);
    s.segmentmarkerlist = static_cast<int*>(malloc(nSegments * sizeof(int)));
    int* seg = s.segmentlist;
    int* segm = s.segmentmarkerlist;
    for (std::size_t i = 0; i < boundaries.size(); ++i) {
      for (std::size_t j = 0; j < boundaries[i].points.size() - 1; ++j) {
        *seg = mapPointToIndex.at(qPointToInt(boundaries[i].points[j]));
        ++seg;
        *seg = mapPointToIndex.at(qPointToInt(boundaries[i].points[j + 1]));
        ++seg;
        // marker id starts at 2, since 0 means not assigned, and 1 may be
        // assigned by Triangle library
        *segm = static_cast<int>(i + 2);
        ++segm;
      }
      if (boundaries[i].isLoop) {
        // connect last point to first point
        *seg = mapPointToIndex.at(qPointToInt(boundaries[i].points.back()));
        ++seg;
        *seg = mapPointToIndex.at(qPointToInt(boundaries[i].points.front()));
        ++seg;
        *segm = static_cast<int>(i + 2);
        ++segm;
      }
    }
  }
}

static void setHoleList(struct triangle::triangulateio& s,
                        const std::vector<QPoint>& holes) {
  auto nHoles = holes.size();
  s.holelist = static_cast<double*>(malloc(2 * holes.size() * sizeof(double)));
  s.numberofholes = static_cast<int>(nHoles);
  double* h = s.holelist;
  for (const auto& hole : holes) {
    *h = hole.x();
    ++h;
    *h = hole.y();
    ++h;
  }
}

static void setRegionList(struct triangle::triangulateio& s,
                          const std::vector<QPoint>& regions) {
  if (regions.empty()) {
    s.numberofregions = 0;
    return;
  }
  s.regionlist =
      static_cast<double*>(malloc(4 * regions.size() * sizeof(double)));
  s.numberofregions = static_cast<int>(regions.size());
  double* r = s.regionlist;
  int i = 1;
  for (const auto& region : regions) {
    *r = region.x();
    ++r;
    *r = region.y();
    ++r;
    *r = static_cast<double>(i);  // compartment index
    ++r;
    *r = 0.0;  // unused
    ++r;
    ++i;
  }
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
      if (neighbours == 1 && largestColIndex == colIndex) {
        // - point has one other colour as neighbour
        // - this colour index is the larger
        setBoundaryPoint(point);
      } else if (neighbours > 1) {
        // - point has multiple colour neighbours
        setBoundaryPoint(point, true);
      }
    }
  }
}

static double triangleArea(const QPoint& a, const QPoint& b, const QPoint& c) {
  // https://en.wikipedia.org/wiki/Shoelace_formula
  return 0.5 * std::fabs(a.x() * b.y() + b.x() * c.y() + c.x() * a.y() -
                         b.x() * a.y() - c.x() * b.y() - a.x() * c.y());
}

Mesh::Mesh(const QImage& image, const std::vector<QPoint>& regions,
           std::size_t maxBoundaryPoints)
    : img(image), maxPoints(maxBoundaryPoints) {
  constructBoundaries();
  constructMesh(regions);
}

// Visvalingam polyline simplification
// NB: very inefficient & not quite right:
// when recalculating triangle, if new one is smaller
// than the one just removed, should use the just-removed value
// for the area - but good enough for now
void Mesh::simplifyBoundary(Boundary& bp) const {
  std::size_t size = bp.points.size();
  double minArea = std::numeric_limits<double>::max();
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
    ++iter;
  }
  if (minArea > 0 && size <= maxPoints) {
    return;
  }
  // remove point with smallest triangle
  bp.points.erase(iterSmallest);
  // repeat
  simplifyBoundary(bp);
}

void Mesh::constructBoundaries() {
  boundaries.clear();

  // add image boundary loop
  boundaries.emplace_back();
  boundaries.back().points.push_back(QPoint(0, 0));
  boundaries.back().points.push_back(QPoint(0, img.height() - 1));
  boundaries.back().points.push_back(QPoint(img.width() - 1, img.height() - 1));
  boundaries.back().points.push_back(QPoint(img.width() - 1, 0));
  boundaries.back().isLoop = true;

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
        qDebug() << "fp start" << fp;
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
          qDebug() << currPoint;
          qDebug() << bbg.isFixed(currPoint);
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
        simplifyBoundary(boundary);
        boundaries.push_back(boundary);
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
      qDebug() << "Start" << startPoint;
      Boundary boundary;
      boundary.isLoop = true;
      QPoint currPoint = startPoint;
      bool finished = false;
      while (!finished) {
        boundary.points.push_back(currPoint);
        qDebug() << currPoint;
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
      simplifyBoundary(boundary);
      boundaries.push_back(boundary);
    }
  } while (foundStartPoint);
}

void Mesh::constructMesh(const std::vector<QPoint>& regions) {
  // construct triangle library input
  triangle::triangulateio in;
  setPointList(in, boundaries, true);
  // setHoleList(in, {QPoint(25, 25), QPoint(24, 24)});
  setRegionList(in, regions);
  triangle::triangulateio out;
  // call triangle library
  triangle::triangulate("zVqpA", &in, &out, nullptr);
  // parse triangle library output
  vertices.clear();
  spdlog::info("Mesh::constructMesh :: {} vertices", out.numberofpoints);
  for (int i = 0; i < out.numberofpoints; ++i) {
    auto x = static_cast<int>(out.pointlist[2 * i]);
    auto y = static_cast<int>(out.pointlist[2 * i + 1]);
    vertices.push_back(QPoint(x, y));
  }
  triangleIDs.clear();
  spdlog::info("Mesh::constructMesh :: {} triangles", out.numberoftriangles);
  std::size_t maxCompIndex = 0;
  for (int i = 0; i < out.numberoftriangles; ++i) {
    auto t0 = static_cast<std::size_t>(out.trianglelist[i * 3]);
    auto t1 = static_cast<std::size_t>(out.trianglelist[i * 3 + 1]);
    auto t2 = static_cast<std::size_t>(out.trianglelist[i * 3 + 2]);
    std::size_t compIndex = 0;
    if (out.numberoftriangleattributes > 0) {
      compIndex = static_cast<std::size_t>(out.triangleattributelist[i]);
    }
    maxCompIndex = std::max(maxCompIndex, compIndex);
    spdlog::info("Mesh::constructMesh :: compIndex {} triangle {} {} {}",
                 compIndex, t0, t1, t2);
    triangleIDs.push_back({compIndex, t0, t1, t2});
  }
  triangles = std::vector<std::vector<QTriangle>>(maxCompIndex + 1,
                                                  std::vector<QTriangle>{});
  for (const auto& t : triangleIDs) {
    triangles[t[0]].push_back({vertices[t[1]], vertices[t[2]], vertices[t[3]]});
  }
}

QString Mesh::getGMSH() const {
  // note: gmsh indexing starts with 1
  // note: gmsh (0,0) is bottom left, in Qt it is top left, so flip y coords
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
                   .arg(vertices[i].x())
                   .arg(img.height() - 1 - vertices[i].y())
                   .arg(0));
  }
  msh.append("$EndNodes\n");
  msh.append("$Elements\n");
  msh.append(QString("%1\n").arg(triangleIDs.size()));
  for (std::size_t i = 0; i < triangleIDs.size(); ++i) {
    msh.append(QString("%1 2 2 %2 %2 %3 %4 %5\n")
                   .arg(i + 1)
                   .arg(triangleIDs[i][0] + 1)
                   .arg(triangleIDs[i][1] + 1)
                   .arg(triangleIDs[i][2] + 1)
                   .arg(triangleIDs[i][3] + 1));
  }
  msh.append("$EndElements\n");
  return msh;
}

}  // namespace mesh

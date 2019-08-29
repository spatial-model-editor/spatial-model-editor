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

Mesh::Mesh(const QImage& image, const std::vector<QPointF>& interiorPoints,
           const std::vector<std::size_t>& maxPoints,
           const std::vector<std::size_t>& maxTriangleArea)
    : img(image),
      compartmentInteriorPoints(interiorPoints),
      boundaryMaxPoints(maxPoints),
      compartmentMaxTriangleArea(maxTriangleArea) {
  boundaries = boundary::constructBoundaries(image);
  SPDLOG_INFO("found {} boundaries", boundaries.size());
  for (const auto& boundary : boundaries) {
    SPDLOG_INFO("  - {} points, loop={}", boundary.points.size(),
                boundary.isLoop);
  }
  if (boundaryMaxPoints.empty()) {
    // if boundary points not specified use default value
    boundaryMaxPoints =
        std::vector<std::size_t>(boundaries.size(), defaultBoundaryMaxPoints);
    SPDLOG_INFO(
        "no boundaryMaxPoints values specified, using default value: {}",
        defaultBoundaryMaxPoints);
  }
  for (std::size_t i = 0; i < boundaries.size(); ++i) {
    boundaries[i].setMaxPoints(boundaryMaxPoints[i]);
  }
  SPDLOG_INFO("simplified {} boundaries", boundaries.size());
  for (const auto& boundary : boundaries) {
    SPDLOG_INFO("  - {} points, loop={}", boundary.points.size(),
                boundary.isLoop);
  }
  if (compartmentMaxTriangleArea.empty()) {
    // if triangle areas not specified use default value
    compartmentMaxTriangleArea = std::vector<std::size_t>(
        interiorPoints.size(), defaultCompartmentMaxTriangleArea);
    SPDLOG_INFO("no max triangle areas specified, using default value: {}",
                defaultCompartmentMaxTriangleArea);
  }
  constructMesh();
}

Mesh::Mesh(const std::vector<double>& inputVertices,
           const std::vector<std::vector<int>>& inputTriangleIndices,
           const std::vector<QPointF>& interiorPoints)
    : compartmentInteriorPoints(interiorPoints) {
  // we don't have the original image, so no boundary info: read only mesh
  readOnlyMesh = true;
  // import vertices
  std::size_t nV = inputVertices.size() / 2;
  vertices.clear();
  vertices.reserve(nV);
  for (std::size_t i = 0; i < nV; ++i) {
    vertices.push_back(QPointF(inputVertices[2 * i], inputVertices[2 * i + 1]));
  }
  // import triangles
  nTriangles = 0;
  triangles = std::vector<std::vector<QTriangleF>>(interiorPoints.size(),
                                                   std::vector<QTriangleF>{});
  triangleIndices = std::vector<std::vector<std::array<std::size_t, 3>>>(
      interiorPoints.size(), std::vector<std::array<std::size_t, 3>>{});
  for (std::size_t compIndex = 0; compIndex < interiorPoints.size();
       ++compIndex) {
    const auto& t = inputTriangleIndices[compIndex];
    std::size_t nT = t.size() / 3;
    triangles[compIndex].reserve(nT);
    triangleIndices[compIndex].reserve(nT);
    for (std::size_t i = 0; i < nT; ++i) {
      auto t0 = static_cast<std::size_t>(t[3 * i]);
      auto t1 = static_cast<std::size_t>(t[3 * i + 1]);
      auto t2 = static_cast<std::size_t>(t[3 * i + 2]);
      triangleIndices[compIndex].push_back({{t0, t1, t2}});
      triangles[compIndex].push_back(
          {{vertices[t0], vertices[t1], vertices[t2]}});
      ++nTriangles;
    }
  }
  SPDLOG_INFO("Imported read-only mesh of {} vertices, {} triangles",
              vertices.size(), nTriangles);
}

bool Mesh::isReadOnly() const { return readOnlyMesh; }

void Mesh::setBoundaryMaxPoints(std::size_t boundaryIndex,
                                std::size_t maxPoints) {
  if (readOnlyMesh) {
    SPDLOG_INFO("mesh is read only, ignoring.");
    return;
  }
  SPDLOG_INFO("boundaryIndex {}: max points {} -> {}", boundaryIndex,
              boundaries.at(boundaryIndex).getMaxPoints(), maxPoints);
  boundaries.at(boundaryIndex).setMaxPoints(maxPoints);
}

std::size_t Mesh::getBoundaryMaxPoints(std::size_t boundaryIndex) const {
  return boundaries.at(boundaryIndex).getMaxPoints();
}

std::vector<std::size_t> Mesh::getBoundaryMaxPoints() const {
  std::vector<std::size_t> v;
  for (const auto& boundary : boundaries) {
    v.push_back(boundary.getMaxPoints());
  }
  return v;
}

void Mesh::setCompartmentMaxTriangleArea(std::size_t compartmentIndex,
                                         std::size_t maxTriangleArea) {
  if (readOnlyMesh) {
    SPDLOG_INFO("mesh is read only, ignoring.");
    return;
  }
  SPDLOG_INFO("compIndex {}: max triangle area {} -> {}", compartmentIndex,
              compartmentMaxTriangleArea.at(compartmentIndex), maxTriangleArea);
  compartmentMaxTriangleArea.at(compartmentIndex) = maxTriangleArea;
  constructMesh();
}

std::size_t Mesh::getCompartmentMaxTriangleArea(
    std::size_t compartmentIndex) const {
  return compartmentMaxTriangleArea.at(compartmentIndex);
}

const std::vector<std::size_t>& Mesh::getCompartmentMaxTriangleArea() const {
  return compartmentMaxTriangleArea;
}

void Mesh::constructMesh() {
  // points may be used by multiple boundary lines,
  // so first construct a set of unique points with map to their index
  // todo: optimise this: we know image size, so can use array instead of map
  //       also (maybe) can assume only first/last points can be used multiple
  //       times
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
  std::vector<triangulate::BoundarySegments> boundarySegmentsVector;
  for (std::size_t i = 0; i < boundaries.size(); ++i) {
    boundarySegmentsVector.emplace_back();
    for (std::size_t j = 0; j < boundaries[i].points.size() - 1; ++j) {
      auto i0 = mapPointToIndex.at(qPointToInt(boundaries[i].points[j]));
      auto i1 = mapPointToIndex.at(qPointToInt(boundaries[i].points[j + 1]));
      boundarySegmentsVector.back().push_back({{i0, i1}});
    }
    if (boundaries[i].isLoop) {
      // connect last point to first point
      auto i0 = mapPointToIndex.at(qPointToInt(boundaries[i].points.back()));
      auto i1 = mapPointToIndex.at(qPointToInt(boundaries[i].points.front()));
      boundarySegmentsVector.back().push_back({{i0, i1}});
    }
  }
  // interior point & max triangle area for each compartment
  std::vector<triangulate::Compartment> compartments;
  for (std::size_t i = 0; i < compartmentInteriorPoints.size(); ++i) {
    compartments.emplace_back(compartmentInteriorPoints[i],
                              compartmentMaxTriangleArea[i]);
  }
  // generate mesh
  triangulate::Triangulate triangulate(boundaryPoints, boundarySegmentsVector,
                                       compartments);
  vertices = triangulate.getPoints();

  // construct triangles & indices for each compartment:
  triangleIndices = std::vector<std::vector<std::array<std::size_t, 3>>>(
      compartments.size(), std::vector<std::array<std::size_t, 3>>{});
  triangles = std::vector<std::vector<QTriangleF>>(compartments.size(),
                                                   std::vector<QTriangleF>{});
  nTriangles = 0;
  for (const auto& t : triangulate.getTriangleIndices()) {
    std::size_t compIndex = t[0] - 1;
    triangleIndices[compIndex].push_back({{t[1], t[2], t[3]}});
    triangles[compIndex].push_back(
        {{vertices[t[1]], vertices[t[2]], vertices[t[3]]}});
    ++nTriangles;
  }
  SPDLOG_INFO("{} vertices, {} triangles", vertices.size(), nTriangles);
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
  // fill triangles in chosen compartment
  for (const auto& t : triangles.at(compartmentIndex)) {
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
  msh.append(QString("%1\n").arg(nTriangles));
  std::size_t triangleIndex = 1;
  std::size_t compartmentIndex = 1;
  for (const auto& comp : triangleIndices) {
    for (const auto& t : comp) {
      msh.append(QString("%1 2 2 %2 %2 %3 %4 %5\n")
                     .arg(triangleIndex)
                     .arg(compartmentIndex)
                     .arg(t[0] + 1)
                     .arg(t[1] + 1)
                     .arg(t[2] + 1));
      ++triangleIndex;
    }
    ++compartmentIndex;
  }

  msh.append("$EndElements\n");
  return msh;
}

}  // namespace mesh

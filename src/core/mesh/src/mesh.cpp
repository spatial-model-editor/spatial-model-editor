#include "mesh.hpp"
#include "boundary.hpp"
#include "logger.hpp"
#include "triangulate.hpp"
#include "utils.hpp"
#include <QColor>
#include <QGradient>
#include <QPainter>
#include <QPen>
#include <QPoint>
#include <QSize>
#include <QtCore>
#include <algorithm>
#include <exception>
#include <utility>

namespace mesh {

QPointF
Mesh::pixelPointToPhysicalPoint(const QPointF &pixelPoint) const noexcept {
  return pixelPoint * pixel + origin;
}

Mesh::Mesh() = default;

Mesh::Mesh(const QImage &image, std::vector<std::size_t> maxPoints,
           std::vector<std::size_t> maxTriangleArea, double pixelWidth,
           const QPointF &originPoint,
           const std::vector<QRgb> &compartmentColours)
    : img(image), origin(originPoint), pixel(pixelWidth),
      boundaryMaxPoints(std::move(maxPoints)),
      compartmentMaxTriangleArea(std::move(maxTriangleArea)) {
  boundaries = std::make_unique<std::vector<Boundary>>(
      constructBoundaries(image, compartmentColours, &compartmentInteriorPoints));
  SPDLOG_INFO("found {} boundaries", boundaries->size());
  for (const auto &boundary : *boundaries) {
    SPDLOG_INFO("  - {} points, loop={}", boundary.getPoints().size(),
                boundary.isLoop());
  }
  if (boundaryMaxPoints.size() != boundaries->size()) {
    // if boundary points not correctly specified use automatic values instead
    SPDLOG_INFO("boundaryMaxPoints has size {}, but there are {} boundaries - "
                "using automatic values",
                boundaryMaxPoints.size(), boundaries->size());
    for (auto &boundary : *boundaries) {
      boundary.setMaxPoints();
    }
  } else {
    for (std::size_t i = 0; i < boundaryMaxPoints.size(); ++i) {
      (*boundaries)[i].setMaxPoints(boundaryMaxPoints[i]);
    }
  }
  SPDLOG_INFO("simplified {} boundaries", boundaries->size());
  for (const auto &boundary : *boundaries) {
    SPDLOG_INFO("  - {} points, loop={}", boundary.getPoints().size(),
                boundary.isLoop());
  }
  if (compartmentMaxTriangleArea.size() != compartmentColours.size()) {
    // if triangle areas not correctly specified use default value
    constexpr std::size_t defaultCompartmentMaxTriangleArea{40};
    compartmentMaxTriangleArea = std::vector<std::size_t>(
        compartmentColours.size(), defaultCompartmentMaxTriangleArea);
    SPDLOG_INFO("no max triangle areas specified, using default value: {}",
                defaultCompartmentMaxTriangleArea);
  }
  constructMesh();
}

Mesh::Mesh(const std::vector<double> &inputVertices,
           const std::vector<std::vector<int>> &inputTriangleIndices,
           const std::vector<std::vector<QPointF>> &interiorPoints)
    : readOnlyMesh(true), compartmentInteriorPoints(interiorPoints) {
  // we don't have the original image, so no boundary info: read only mesh
  // import vertices: supplied as list of doubles, two for each vertex
  std::size_t nV = inputVertices.size() / 2;
  vertices.clear();
  vertices.reserve(nV);
  for (std::size_t i = 0; i < nV; ++i) {
    vertices.emplace_back(inputVertices[2 * i], inputVertices[2 * i + 1]);
  }
  // import triangles: supplies as list of ints, three for each triangle
  nTriangles = 0;
  triangles = std::vector<std::vector<QTriangleF>>(interiorPoints.size(),
                                                   std::vector<QTriangleF>{});
  triangleIndices = std::vector<std::vector<TriangulateTriangleIndex>>(
      interiorPoints.size(), std::vector<TriangulateTriangleIndex>{});
  for (std::size_t compIndex = 0; compIndex < interiorPoints.size();
       ++compIndex) {
    const auto &t = inputTriangleIndices[compIndex];
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

Mesh::~Mesh() = default;

bool Mesh::isReadOnly() const { return readOnlyMesh; }

bool Mesh::isValid() const { return validMesh; }

std::size_t Mesh::getNumBoundaries() const { return boundaries->size(); }

void Mesh::setBoundaryMaxPoints(std::size_t boundaryIndex,
                                std::size_t maxPoints) {
  if (readOnlyMesh) {
    SPDLOG_INFO("mesh is read only, ignoring.");
    return;
  }
  SPDLOG_INFO("boundaryIndex {}: max points {} -> {}", boundaryIndex,
              (*boundaries)[boundaryIndex].getMaxPoints(), maxPoints);
  (*boundaries)[boundaryIndex].setMaxPoints(maxPoints);
}

std::size_t Mesh::getBoundaryMaxPoints(std::size_t boundaryIndex) const {
  return (*boundaries)[boundaryIndex].getMaxPoints();
}

std::vector<std::size_t> Mesh::getBoundaryMaxPoints() const {
  std::vector<std::size_t> v(boundaries->size());
  std::transform(boundaries->cbegin(), boundaries->cend(), v.begin(),
                 [](const auto &b) { return b.getMaxPoints(); });
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

std::size_t
Mesh::getCompartmentMaxTriangleArea(std::size_t compartmentIndex) const {
  return compartmentMaxTriangleArea.at(compartmentIndex);
}

const std::vector<std::size_t> &Mesh::getCompartmentMaxTriangleArea() const {
  return compartmentMaxTriangleArea;
}

const std::vector<std::vector<QPointF>> &
Mesh::getCompartmentInteriorPoints() const {
  return compartmentInteriorPoints;
}

void Mesh::setPhysicalGeometry(double pixelWidth, const QPointF &originPoint) {
  pixel = pixelWidth;
  origin = originPoint;
}

std::vector<double> Mesh::getVerticesAsFlatArray() const {
  // convert from pixels to physical coordinates
  std::vector<double> v;
  v.reserve(vertices.size() * 2);
  for (const auto &pixelPoint : vertices) {
    auto physicalPoint = pixelPointToPhysicalPoint(pixelPoint);
    v.push_back(physicalPoint.x());
    v.push_back(physicalPoint.y());
  }
  return v;
}

std::vector<int>
Mesh::getTriangleIndicesAsFlatArray(std::size_t compartmentIndex) const {
  std::vector<int> out;
  const auto &indices = triangleIndices[compartmentIndex];
  out.reserve(indices.size() * 3);
  for (const auto &t : indices) {
    for (std::size_t ti : t) {
      out.push_back(static_cast<int>(ti));
    }
  }
  return out;
}

const std::vector<std::vector<TriangulateTriangleIndex>> &
Mesh::getTriangleIndices() const {
  return triangleIndices;
}

const std::vector<std::vector<QTriangleF>> &Mesh::getTriangles() const {
  return triangles;
}

static std::size_t getOrInsertFPIndex(const QPoint &p,
                                      std::vector<QPoint> &fps) {
  // return index of item in points that matches p
  SPDLOG_TRACE("looking for point ({}, {})", p.x(), p.y());
  for (std::size_t i = 0; i < fps.size(); ++i) {
    if (fps[i] == p) {
      SPDLOG_TRACE("  -> found [{}] : ({}, {})", i, fps[i].x(), fps[i].y());
      return i;
    }
  }
  // if not found: add p to vector and return its index
  SPDLOG_TRACE("  -> added new point");
  fps.push_back(p);
  return fps.size() - 1;
}

static TriangulateBoundaries
constructTriangulateBoundaries(const std::vector<Boundary> &boundaries) {
  TriangulateBoundaries tid;
  std::size_t nPointsUpperBound = 0;
  for (const auto &boundary : boundaries) {
    nPointsUpperBound += boundary.getPoints().size();
  }
  tid.vertices.reserve(nPointsUpperBound);
  // first add fixed points
  std::vector<QPoint> fps;
  fps.reserve(2 * boundaries.size());
  for (const auto &boundary : boundaries) {
    if (!boundary.isLoop()) {
      getOrInsertFPIndex(boundary.getPoints().front(), fps);
      getOrInsertFPIndex(boundary.getPoints().back(), fps);
    }
  }
  for (const auto &fp : fps) {
    SPDLOG_TRACE("- fp ({},{})", fp.x(), fp.y());
    tid.vertices.emplace_back(fp);
  }

  // for each segment in each boundary line, add the QPoints if not already
  // present, and add the pair of point indices to the list of segment indices
  std::size_t currentIndex = tid.vertices.size() - 1;
  for (const auto &boundary : boundaries) {
    SPDLOG_TRACE("{}-point boundary", boundary.getPoints().size());
    SPDLOG_TRACE("  - loop: {}", boundary.isLoop());
    const auto &points = boundary.getPoints();
    auto &segments = tid.boundaries.emplace_back();
    // do first segment
    if (boundary.isLoop()) {
      tid.vertices.emplace_back(points[0]);
      ++currentIndex;
      tid.vertices.emplace_back(points[1]);
      ++currentIndex;
      segments.push_back({currentIndex - 1, currentIndex});
    } else if (points.size() == 2) {
      segments.push_back({getOrInsertFPIndex(points.front(), fps),
                          getOrInsertFPIndex(points.back(), fps)});
    } else {
      tid.vertices.emplace_back(points[1]);
      ++currentIndex;
      segments.push_back(
          {getOrInsertFPIndex(points.front(), fps), currentIndex});
    }
    // do intermediate segments
    for (std::size_t j = 2; j < points.size() - 1; ++j) {
      tid.vertices.emplace_back(points[j]);
      ++currentIndex;
      segments.push_back({currentIndex - 1, currentIndex});
    }
    // do last segment
    if (boundary.isLoop()) {
      ++currentIndex;
      tid.vertices.emplace_back(points.back());
      segments.push_back({currentIndex - 1, currentIndex});
      // for loops: also connect last point to first point
      segments.push_back({currentIndex, segments.front().start});
    } else if (points.size() > 2) {
      segments.push_back(
          {currentIndex, getOrInsertFPIndex(points.back(), fps)});
    }
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_TRACE
    for (const auto &seg : segments) {
      SPDLOG_TRACE("- seg {}->{} | ({},{})->({},{})", seg.start, seg.end,
                   tid.vertices[seg.start].x(), tid.vertices[seg.start].y(),
                   tid.vertices[seg.end].x(), tid.vertices[seg.end].y());
    }
#endif
  }
  return tid;
}

void Mesh::constructMesh() {
  try {
    auto tid = constructTriangulateBoundaries(*boundaries);

    // add interior point & max triangle area for each compartment
    for (std::size_t i = 0; i < compartmentInteriorPoints.size(); ++i) {
      tid.compartments.push_back(
          {compartmentInteriorPoints[i],
           static_cast<double>(compartmentMaxTriangleArea[i])});
    }
    Triangulate triangulate(tid);
    vertices = triangulate.getPoints();
    triangleIndices = triangulate.getTriangleIndices();

    // construct triangles for each compartment:
    nTriangles = 0;
    triangles.clear();
    for (const auto &compartmentTriangleIndices : triangleIndices) {
      nTriangles += compartmentTriangleIndices.size();
      auto &compTriangles = triangles.emplace_back();
      SPDLOG_TRACE("  - adding triangle compartment");
      for (const auto &t : compartmentTriangleIndices) {
        compTriangles.push_back(
            {{vertices[t[0]], vertices[t[1]], vertices[t[2]]}});
      }
    }
  } catch (const std::exception &e) {
    SPDLOG_WARN("constructMesh failed with exception: {}", e.what());
    validMesh = false;
    vertices.clear();
    triangleIndices.clear();
    triangles.clear();
  }
}

static double getScaleFactor(const QImage &img, const QSize &size,
                             const QPointF &offset) {
  auto Swidth = static_cast<double>(size.width()) - 2 * offset.x();
  auto Sheight = static_cast<double>(size.height()) - 2 * offset.y();
  auto Iwidth = static_cast<double>(img.width());
  auto Iheight = static_cast<double>(img.height());
  return std::min(Swidth / Iwidth, Sheight / Iheight);
}

std::pair<QImage, QImage>
Mesh::getBoundariesImages(const QSize &size,
                          std::size_t boldBoundaryIndex) const {
  constexpr int defaultPenSize = 2;
  constexpr int boldPenSize = 5;
  constexpr int maskPenSize = 15;
  QPointF offset(5.0, 5.0);
  double scaleFactor = getScaleFactor(img, size, offset);

  // construct boundary image
  QImage boundaryImage(
      static_cast<int>(scaleFactor * img.width() + 2 * offset.x()),
      static_cast<int>(scaleFactor * img.height() + 2 * offset.y()),
      QImage::Format_ARGB32_Premultiplied);
  boundaryImage.fill(qRgba(0, 0, 0, 0));

  QImage maskImage(boundaryImage.size(), QImage::Format_ARGB32_Premultiplied);
  maskImage.fill(qRgb(255, 255, 255));

  QPainter painter(&boundaryImage);
  painter.setRenderHint(QPainter::Antialiasing);

  QPainter pMask(&maskImage);

  // draw boundary lines
  for (std::size_t k = 0; k < boundaries->size(); ++k) {
    const auto &points = (*boundaries)[k].getPoints();
    int penSize = defaultPenSize;
    if (k == boldBoundaryIndex) {
      penSize = boldPenSize;
    }
    painter.setPen(QPen(utils::indexedColours()[k], penSize));
    pMask.setPen(QPen(QColor(0, 0, static_cast<int>(k)), maskPenSize));
    for (std::size_t i = 0; i < points.size() - 1; ++i) {
      auto p1 = points[i] * scaleFactor + offset;
      auto p2 = points[i + 1] * scaleFactor + offset;
      painter.drawEllipse(p1, penSize, penSize);
      painter.drawLine(p1, p2);
      pMask.drawLine(p1, p2);
    }
    painter.drawEllipse(points.back() * scaleFactor + offset, penSize, penSize);
    if ((*boundaries)[k].isLoop()) {
      auto p1 = points.back() * scaleFactor + offset;
      auto p2 = points.front() * scaleFactor + offset;
      painter.drawLine(p1, p2);
      pMask.drawLine(p1, p2);
    }
  }
  painter.end();
  pMask.end();
  // flip image on y-axis, to change (0,0) from bottom-left to top-left corner
  return std::make_pair(boundaryImage.mirrored(false, true),
                        maskImage.mirrored(false, true));
}

std::pair<QImage, QImage>
Mesh::getMeshImages(const QSize &size, std::size_t compartmentIndex) const {
  std::pair<QImage, QImage> imgPair;
  auto &[meshImage, maskImage] = imgPair;
  QPointF offset(5.0, 5.0);
  double scaleFactor = getScaleFactor(img, size, offset);
  // construct mesh image
  meshImage =
      QImage(static_cast<int>(scaleFactor * img.width() + 2 * offset.x()),
             static_cast<int>(scaleFactor * img.height() + 2 * offset.y()),
             QImage::Format_ARGB32_Premultiplied);
  meshImage.fill(QColor(0, 0, 0, 0));
  QPainter p(&meshImage);
  p.setRenderHint(QPainter::Antialiasing);
  // construct mask image
  maskImage = QImage(meshImage.size(), QImage::Format_RGB32);
  maskImage.fill(QColor(255, 255, 255).rgba());
  QPainter pMask(&maskImage);
  // draw triangles
  for (std::size_t k = 0; k < triangles.size(); ++k) {
    if (k == compartmentIndex) {
      // fill triangles in chosen compartment & outline with bold lines
      p.setPen(QPen(Qt::black, 2));
      p.setBrush(QBrush(QColor(235, 235, 255)));
    } else {
      // outline all other triangles with gray lines
      p.setPen(QPen(Qt::gray, 1));
      p.setBrush({});
    }
    pMask.setBrush(QBrush(QColor(0, 0, static_cast<int>(k))));
    for (const auto &t : triangles[k]) {
      std::array<QPointF, 3> points;
      for (std::size_t i = 0; i < 3; ++i) {
        points[i] = t[i] * scaleFactor + offset;
      }
      p.drawConvexPolygon(points.data(), 3);
      pMask.drawConvexPolygon(points.data(), 3);
    }
  }
  pMask.end();
  // draw vertices
  p.setPen(QPen(Qt::red, 3));
  for (const auto &v : vertices) {
    p.drawPoint(v * scaleFactor + offset);
  }
  p.end();
  meshImage = meshImage.mirrored(false, true);
  maskImage = maskImage.mirrored(false, true);
  return imgPair;
}

QString Mesh::getGMSH(const std::unordered_set<int> &gmshCompIndices) const {
  // note: gmsh indexing starts with 1, so we need to add 1 to all indices
  // meshing is done in terms of pixels, to convert to physical points:
  //   - rescale each vertex by a factor pixel
  //   - add origin to each vertex
  QString msh;
  msh.append("$MeshFormat\n");
  msh.append("2.2 0 8\n");
  msh.append("$EndMeshFormat\n");
  msh.append("$Nodes\n");
  msh.append(QString("%1\n").arg(vertices.size()));
  for (std::size_t i = 0; i < vertices.size(); ++i) {
    auto physicalPoint = pixelPointToPhysicalPoint(vertices[i]);
    msh.append(QString("%1 %2 %3 %4\n")
                   .arg(i + 1)
                   .arg(utils::dblToQStr(physicalPoint.x()))
                   .arg(utils::dblToQStr(physicalPoint.y()))
                   .arg(0));
  }
  msh.append("$EndNodes\n");
  msh.append("$Elements\n");
  std::size_t nElem = 0;
  std::size_t compartmentIndex = 1;
  for (const auto &comp : triangleIndices) {
    if (gmshCompIndices.empty() ||
        gmshCompIndices.find(static_cast<int>(compartmentIndex)) !=
            gmshCompIndices.cend()) {
      SPDLOG_TRACE("Adding compartment of triangles, index: {}",
                   compartmentIndex);
      nElem += comp.size();
    }
    compartmentIndex++;
  }
  msh.append(QString("%1\n").arg(nElem));
  std::size_t elementIndex = 1;
  compartmentIndex = 1;
  std::size_t outputCompartmentIndex = 1;
  for (const auto &comp : triangleIndices) {
    if (gmshCompIndices.empty() ||
        gmshCompIndices.find(static_cast<int>(compartmentIndex)) !=
            gmshCompIndices.cend()) {
      SPDLOG_TRACE("Writing triangles for compartment index: {}",
                   compartmentIndex);
      for (const auto &t : comp) {
        msh.append(QString("%1 2 2 %2 %2 %3 %4 %5\n")
                       .arg(elementIndex)
                       .arg(outputCompartmentIndex)
                       .arg(t[0] + 1)
                       .arg(t[1] + 1)
                       .arg(t[2] + 1));
        ++elementIndex;
      }
      ++outputCompartmentIndex;
    }
    ++compartmentIndex;
  }
  msh.append("$EndElements\n");
  return msh;
}

} // namespace mesh

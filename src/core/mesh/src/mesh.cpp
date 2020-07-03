#include "mesh.hpp"

#include <QPainter>
#include <utility>

#include "image_boundaries.hpp"
#include "logger.hpp"
#include "triangulate.hpp"
#include "utils.hpp"

namespace mesh {

QPointF
Mesh::pixelPointToPhysicalPoint(const QPointF &pixelPoint) const noexcept {
  return pixelPoint * pixel + origin;
}

Mesh::Mesh() = default;

Mesh::Mesh(
    const QImage &image, const std::vector<QPointF> &interiorPoints,
    std::vector<std::size_t> maxPoints,
    std::vector<std::size_t> maxTriangleArea,
    const std::vector<std::pair<std::string, ColourPair>> &membraneColourPairs,
    const std::vector<double> &membraneWidths, double pixelWidth,
    const QPointF &originPoint, const std::vector<QRgb> &compartmentColours)
    : img(image), origin(originPoint), pixel(pixelWidth),
      compartmentInteriorPoints(interiorPoints),
      boundaryMaxPoints(std::move(maxPoints)),
      compartmentMaxTriangleArea(std::move(maxTriangleArea)),
      imageBoundaries{std::make_unique<ImageBoundaries>(
          image, compartmentColours, membraneColourPairs)} {
  const auto &boundaries = imageBoundaries->getBoundaries();
  SPDLOG_INFO("found {} boundaries", boundaries.size());
  for (const auto &boundary : boundaries) {
    SPDLOG_INFO("  - {} points, loop={}, membrane={} [{}]",
                boundary.getPoints().size(), boundary.isLoop(),
                boundary.isMembrane(), boundary.getMembraneId());
  }
  if (boundaryMaxPoints.size() != boundaries.size()) {
    // if boundary points not correctly specified use automatic values instead
    SPDLOG_INFO("boundaryMaxPoints has size {}, but there are {} boundaries - "
                "using automatic values",
                boundaryMaxPoints.size(), boundaries.size());
    boundaryMaxPoints = imageBoundaries->setAutoMaxPoints();
  } else {
    imageBoundaries->setMaxPoints(boundaryMaxPoints);
  }
  SPDLOG_INFO("simplified {} boundaries", boundaries.size());
  for (const auto &boundary : boundaries) {
    SPDLOG_INFO("  - {} points, loop={}, membrane={}",
                boundary.getPoints().size(), boundary.isLoop(),
                boundary.isMembrane());
  }
  if (compartmentMaxTriangleArea.empty()) {
    // if triangle areas not specified use default value
    compartmentMaxTriangleArea = std::vector<std::size_t>(
        interiorPoints.size(), defaultCompartmentMaxTriangleArea);
    SPDLOG_INFO("no max triangle areas specified, using default value: {}",
                defaultCompartmentMaxTriangleArea);
  }
  // set membrane widths if supplied & valid
  if (!membraneWidths.empty() && membraneWidths.size() == boundaries.size()) {
    imageBoundaries->setMembraneWidths(membraneWidths);
  }
  try {
    constructMesh();
  } catch (const std::exception &e) {
    SPDLOG_WARN("constructMesh failed with exception: {}", e.what());
    validMesh = false;
  }
}

Mesh::Mesh(const std::vector<double> &inputVertices,
           const std::vector<std::vector<int>> &inputTriangleIndices,
           const std::vector<QPointF> &interiorPoints)
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
  triangleIndices = std::vector<std::vector<TriangleIndex>>(
      interiorPoints.size(), std::vector<TriangleIndex>{});
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

std::size_t Mesh::getNumBoundaries() const {
  return imageBoundaries->getBoundaries().size();
}

bool Mesh::isMembrane(std::size_t boundaryIndex) const {
  return imageBoundaries->getBoundaries()[boundaryIndex].isMembrane();
}

void Mesh::setBoundaryMaxPoints(std::size_t boundaryIndex,
                                std::size_t maxPoints) {
  if (readOnlyMesh) {
    SPDLOG_INFO("mesh is read only, ignoring.");
    return;
  }
  SPDLOG_INFO("boundaryIndex {}: max points {} -> {}", boundaryIndex,
              imageBoundaries->getBoundaries()[boundaryIndex].getMaxPoints(),
              maxPoints);
  imageBoundaries->setMaxPoints(boundaryIndex, maxPoints);
}

std::size_t Mesh::getBoundaryMaxPoints(std::size_t boundaryIndex) const {
  return imageBoundaries->getBoundaries()[boundaryIndex].getMaxPoints();
}

std::vector<std::size_t> Mesh::getBoundaryMaxPoints() const {
  const auto &boundaries = imageBoundaries->getBoundaries();
  std::vector<std::size_t> v(boundaries.size());
  std::transform(boundaries.cbegin(), boundaries.cend(), v.begin(),
                 [](const auto &b) { return b.getMaxPoints(); });
  return v;
}

void Mesh::setBoundaryWidth(std::size_t boundaryIndex, double width) {
  if (readOnlyMesh) {
    SPDLOG_INFO("mesh is read only, ignoring.");
    return;
  }
  SPDLOG_INFO(
      "boundaryIndex {}: width {} -> {}", boundaryIndex,
      imageBoundaries->getBoundaries()[boundaryIndex].getMembraneWidth(),
      width);
  imageBoundaries->setMembraneWidth(boundaryIndex, width);
}

double Mesh::getBoundaryWidth(std::size_t boundaryIndex) const {
  return imageBoundaries->getBoundaries()[boundaryIndex].getMembraneWidth();
}

std::vector<double> Mesh::getBoundaryWidths() const {
  const auto &boundaries = imageBoundaries->getBoundaries();
  std::vector<double> v(boundaries.size(), 0.0);
  std::transform(boundaries.cbegin(), boundaries.cend(), v.begin(),
                 [](const auto &b) { return b.getMembraneWidth(); });
  return v;
}

double Mesh::getMembraneWidth(const std::string &membraneID) const {
  const auto &boundaries = imageBoundaries->getBoundaries();
  auto iter = std::find_if(boundaries.cbegin(), boundaries.cend(),
                           [membraneID](const Boundary &b) {
                             return b.getMembraneId() == membraneID;
                           });
  if (iter != boundaries.cend()) {
    return iter->getMembraneWidth();
  }
  SPDLOG_ERROR("Boundary for Membrane {} not found", membraneID);
  SPDLOG_ERROR("  -> using default width 1");
  return 1;
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

void Mesh::setPhysicalGeometry(double pixelWidth, const QPointF &originPoint) {
  pixel = pixelWidth;
  origin = originPoint;
}

std::vector<double> Mesh::getVertices() const {
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

std::vector<int> Mesh::getTriangleIndices(std::size_t compartmentIndex) const {
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

const std::vector<std::vector<QTriangleF>> &Mesh::getTriangles() const {
  return triangles;
}

static TriangulateBoundaries
constructTriangulateBoundaries(const ImageBoundaries *imageBoundaries) {
  TriangulateBoundaries tid;
  const auto &boundaries = imageBoundaries->getBoundaries();
  std::size_t nPointsUpperBound = 0;
  for (const auto &boundary : boundaries) {
    nPointsUpperBound += boundary.getPoints().size();
  }
  tid.boundaryPoints.reserve(nPointsUpperBound);
  // first add fixed points
  for (const auto &fp : imageBoundaries->getFixedPoints()) {
    SPDLOG_TRACE("- fp ({},{})", fp.point.x(), fp.point.y());
    tid.boundaryPoints.emplace_back(fp.point);
  }

  // for each segment in each boundary line, add the QPoints if not already
  // present, and add the pair of point indices to the list of segment indices
  std::size_t currentIndex = tid.boundaryPoints.size() - 1;
  for (const auto &boundary : boundaries) {
    SPDLOG_TRACE("{}-point boundary", boundary.getPoints().size());
    SPDLOG_TRACE("  - loop: {}", boundary.isLoop());
    SPDLOG_TRACE("  - membrane: {}", boundary.isMembrane());
    const auto &points = boundary.getPoints();
    auto &segments = tid.boundaries.emplace_back();
    // do first segment
    if (boundary.isLoop()) {
      tid.boundaryPoints.emplace_back(points[0]);
      ++currentIndex;
      tid.boundaryPoints.emplace_back(points[1]);
      ++currentIndex;
      segments.push_back({currentIndex - 1, currentIndex});
    } else if (points.size() == 2) {
      segments.push_back({boundary.getFpIndices().startPoint,
                          boundary.getFpIndices().endPoint});
    } else {
      tid.boundaryPoints.emplace_back(points[1]);
      ++currentIndex;
      segments.push_back({boundary.getFpIndices().startPoint, currentIndex});
    }
    // do intermediate segments
    for (std::size_t j = 2; j < points.size() - 1; ++j) {
      tid.boundaryPoints.emplace_back(points[j]);
      ++currentIndex;
      segments.push_back({currentIndex - 1, currentIndex});
    }
    // do last segment
    if (boundary.isLoop()) {
      ++currentIndex;
      tid.boundaryPoints.emplace_back(points.back());
      segments.push_back({currentIndex - 1, currentIndex});
      // for loops: also connect last point to first point
      segments.push_back({currentIndex, segments.front().start});
    } else if (points.size() > 2) {
      segments.push_back({currentIndex, boundary.getFpIndices().endPoint});
    }
    for (const auto &seg : segments) {
      SPDLOG_TRACE(
          "- seg {}->{} | ({},{})->({},{})", seg.start, seg.end,
          tid.boundaryPoints[seg.start].x(), tid.boundaryPoints[seg.start].y(),
          tid.boundaryPoints[seg.end].x(), tid.boundaryPoints[seg.end].y());
    }
  }

  // boundary index and width for each membrane
  for (std::size_t i = 0; i < boundaries.size(); ++i) {
    const auto &b = boundaries[i];
    auto &bp = tid.boundaryProperties.emplace_back();
    bp.boundaryIndex = i;
    bp.width = b.getMembraneWidth();
    bp.isLoop = b.isLoop();
    bp.isMembrane = b.isMembrane();
    bp.innerFPIndices = {b.getInnerFpIndices().startPoint,
                         b.getInnerFpIndices().endPoint};
    bp.outerFPIndices = {b.getOuterFpIndices().startPoint,
                         b.getOuterFpIndices().endPoint};
  }
  return tid;
}

static TriangulateFixedPoints
constructTriangulateFixedPoints(const ImageBoundaries *imageBoundaries) {
  TriangulateFixedPoints tfp;
  tfp.nFPs = imageBoundaries->getFixedPoints().size();
  tfp.newFPs = imageBoundaries->getNewFixedPoints();
  return tfp;
}

void Mesh::constructMesh() {
  auto tid = constructTriangulateBoundaries(imageBoundaries.get());
  auto tfp = constructTriangulateFixedPoints(imageBoundaries.get());

  // add interior point & max triangle area for each compartment
  for (std::size_t i = 0; i < compartmentInteriorPoints.size(); ++i) {
    tid.compartments.push_back(
        {compartmentInteriorPoints[i],
         static_cast<double>(compartmentMaxTriangleArea[i])});
  }
  Triangulate triangulate(tid, tfp);
  vertices = triangulate.getPoints();
  triangleIndices = triangulate.getTriangleIndices();
  rectangleIndices = triangulate.getRectangleIndices();

  // construct triangles for each compartment:
  nTriangles = 0;
  triangles.clear();
  for (const auto &compartmentTriangleIndices : triangleIndices) {
    nTriangles += compartmentTriangleIndices.size();
    auto &compTriangles = triangles.emplace_back();
    for (const auto &t : compartmentTriangleIndices) {
      compTriangles.push_back(
          {{vertices[t[0]], vertices[t[1]], vertices[t[2]]}});
    }
  }

  // construct rectangles for each membrane:
  nRectangles = 0;
  rectangles.clear();
  for (const auto &membraneRectangleIndices : rectangleIndices) {
    nRectangles += membraneRectangleIndices.size();
    auto &membRectangles = rectangles.emplace_back();
    for (const auto &r : membraneRectangleIndices) {
      membRectangles.push_back(
          {{vertices[r[0]], vertices[r[1]], vertices[r[2]], vertices[r[3]]}});
    }
  }
  SPDLOG_INFO("{} vertices, {} triangles, {} rectangles", vertices.size(),
              nTriangles, nRectangles);
}

static double getScaleFactor(const QImage &img, const QSize &size) {
  auto Swidth = static_cast<double>(size.width());
  auto Sheight = static_cast<double>(size.height());
  auto Iwidth = static_cast<double>(img.width());
  auto Iheight = static_cast<double>(img.height());
  return std::min(Swidth / Iwidth, Sheight / Iheight);
}

const QImage &Mesh::getBoundaryPixelsImage() const {
  return imageBoundaries->getBoundaryPixelsImage();
}

std::pair<QImage, QImage>
Mesh::getBoundariesImages(const QSize &size,
                          std::size_t boldBoundaryIndex) const {
  constexpr int defaultPenSize = 2;
  constexpr int boldPenSize = 5;
  constexpr int maskPenSize = 15;
  double scaleFactor = getScaleFactor(img, size);

  // construct boundary image
  QImage boundaryImage(static_cast<int>(scaleFactor * img.width()),
                       static_cast<int>(scaleFactor * img.height()),
                       QImage::Format_ARGB32_Premultiplied);
  boundaryImage.fill(QColor(0, 0, 0, 0).rgba());

  QImage maskImage(boundaryImage.size(), QImage::Format_ARGB32_Premultiplied);
  maskImage.fill(QColor(255, 255, 255).rgba());

  QPainter painter(&boundaryImage);
  painter.setRenderHint(QPainter::Antialiasing);

  QPainter pMask(&maskImage);

  // draw boundary lines
  const auto &boundaries = imageBoundaries->getBoundaries();
  for (std::size_t k = 0; k < boundaries.size(); ++k) {
    const auto &points = boundaries[k].getInnerPoints();
    int penSize = defaultPenSize;
    if (k == boldBoundaryIndex) {
      penSize = boldPenSize;
    }
    painter.setPen(QPen(utils::indexedColours()[k], penSize));
    pMask.setPen(QPen(QColor(0, 0, static_cast<int>(k)), maskPenSize));
    for (std::size_t i = 0; i < points.size() - 1; ++i) {
      auto p1 = points[i] * scaleFactor;
      auto p2 = points[i + 1] * scaleFactor;
      painter.drawEllipse(p1, penSize, penSize);
      painter.drawLine(p1, p2);
      pMask.drawLine(p1, p2);
    }
    painter.drawEllipse(points.back() * scaleFactor, penSize, penSize);
    if (boundaries[k].isLoop()) {
      auto p1 = points.back() * scaleFactor;
      auto p2 = points.front() * scaleFactor;
      painter.drawLine(p1, p2);
      pMask.drawLine(p1, p2);
    }
    if (boundaries[k].isMembrane()) {
      const auto &outerPoints = boundaries[k].getOuterPoints();
      for (std::size_t i = 0; i < points.size() - 1; ++i) {
        auto p1 = outerPoints[i] * scaleFactor;
        auto p2 = outerPoints[i + 1] * scaleFactor;
        painter.drawEllipse(p1, penSize, penSize);
        painter.drawLine(p1, p2);
        pMask.drawLine(p1, p2);
      }
      painter.drawEllipse(outerPoints.back() * scaleFactor, penSize, penSize);
      if (boundaries[k].isLoop()) {
        auto p1 = outerPoints.back() * scaleFactor;
        auto p2 = outerPoints.front() * scaleFactor;
        painter.drawLine(p1, p2);
        pMask.drawLine(p1, p2);
      }
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
  double scaleFactor = getScaleFactor(img, size);
  // construct mesh image
  meshImage = QImage(static_cast<int>(scaleFactor * img.width()),
                     static_cast<int>(scaleFactor * img.height()),
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
        points[i] = t[i] * scaleFactor;
      }
      p.drawConvexPolygon(points.data(), 3);
      pMask.drawConvexPolygon(points.data(), 3);
    }
  }
  pMask.end();
  // draw any membrane rectangles
  p.setPen(QPen(Qt::gray, 1));
  p.setBrush({});
  for (const auto &membraneRectangles : rectangles) {
    for (const auto &r : membraneRectangles) {
      p.drawLine(r[0] * scaleFactor, r[3] * scaleFactor);
    }
    p.drawLine(membraneRectangles.back()[1] * scaleFactor,
               membraneRectangles.back()[2] * scaleFactor);
  }
  // draw vertices
  p.setPen(QPen(Qt::red, 3));
  for (const auto &v : vertices) {
    p.drawPoint(v * scaleFactor);
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
    msh.append(QString("%1 %2 %3 %4\n")
                   .arg(i + 1)
                   .arg(vertices[i].x() * pixel + origin.x())
                   .arg(vertices[i].y() * pixel + origin.y())
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
      nElem += comp.size();
    }
    compartmentIndex++;
  }
  for (const auto &memb : rectangleIndices) {
    if (gmshCompIndices.empty() ||
        gmshCompIndices.find(static_cast<int>(compartmentIndex)) !=
            gmshCompIndices.cend()) {
      nElem += memb.size();
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
  for (const auto &memb : rectangleIndices) {
    if (gmshCompIndices.empty() ||
        gmshCompIndices.find(static_cast<int>(compartmentIndex)) !=
            gmshCompIndices.cend()) {
      for (const auto &r : memb) {
        msh.append(QString("%1 3 2 %2 %2 %3 %4 %5 %6\n")
                       .arg(elementIndex)
                       .arg(outputCompartmentIndex)
                       .arg(r[0] + 1)
                       .arg(r[1] + 1)
                       .arg(r[2] + 1)
                       .arg(r[3] + 1));
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

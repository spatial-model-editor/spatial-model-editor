#include "mesh.hpp"
#include "boundary.hpp"
#include "interior_point.hpp"
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

namespace sme::mesh {

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
      constructBoundaries(image, compartmentColours));
  compartmentInteriorPoints = getInteriorPoints(image, compartmentColours);
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

Mesh::~Mesh() = default;

bool Mesh::isValid() const { return validMesh; }

const std::string &Mesh::getErrorMessage() const { return errorMessage; };

std::size_t Mesh::getNumBoundaries() const { return boundaries->size(); }

void Mesh::setBoundaryMaxPoints(std::size_t boundaryIndex,
                                std::size_t maxPoints) {
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

void Mesh::constructMesh() {
  try {
    Triangulate triangulate(*boundaries, compartmentInteriorPoints,
                            compartmentMaxTriangleArea);
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
    validMesh = true;
    errorMessage.clear();
  } catch (const std::exception &e) {
    validMesh = false;
    errorMessage = e.what();
    SPDLOG_WARN("constructMesh failed with exception: {}", errorMessage);
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

QString Mesh::getGMSH() const {
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
    SPDLOG_TRACE("Adding compartment of triangles, index: {}",
                 compartmentIndex);
    nElem += comp.size();
    compartmentIndex++;
  }
  msh.append(QString("%1\n").arg(nElem));
  std::size_t elementIndex{1};
  compartmentIndex = 1;
  for (const auto &comp : triangleIndices) {
    SPDLOG_TRACE("Writing triangles for compartment index: {}",
                 compartmentIndex);
    for (const auto &t : comp) {
      msh.append(QString("%1 2 2 %2 %2 %3 %4 %5\n")
                     .arg(elementIndex)
                     .arg(compartmentIndex)
                     .arg(t[0] + 1)
                     .arg(t[1] + 1)
                     .arg(t[2] + 1));
      ++elementIndex;
    }
    ++compartmentIndex;
  }
  msh.append("$EndElements\n");
  return msh;
}

} // namespace sme::mesh

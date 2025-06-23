#include "sme/mesh2d.hpp"
#include "boundaries.hpp"
#include "interior_point.hpp"
#include "sme/logger.hpp"
#include "sme/utils.hpp"
#include "triangulate.hpp"
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
Mesh2d::pixelPointToPhysicalPoint(const QPointF &pixelPoint) const noexcept {
  return origin + QPointF(pixelPoint.x() * pixel.width(),
                          pixelPoint.y() * pixel.height());
}

Mesh2d::Mesh2d() = default;

Mesh2d::Mesh2d(const QImage &image, std::vector<std::size_t> maxPoints,
               std::vector<std::size_t> maxTriangleArea,
               const common::VolumeF &voxelSize,
               const common::VoxelF &originPoint,
               const std::vector<QRgb> &compartmentColors,
               std::size_t boundarySimplificationType)
    : img(image), origin(originPoint.p),
      pixel(voxelSize.width(), voxelSize.height()),
      boundaryMaxPoints(std::move(maxPoints)),
      compartmentMaxTriangleArea(std::move(maxTriangleArea)),
      boundaries{std::make_unique<Boundaries>(image, compartmentColors,
                                              boundarySimplificationType)},
      compartmentInteriorPoints{getInteriorPoints(image, compartmentColors)} {
  SPDLOG_INFO("found {} boundaries", boundaries->size());
  for (const auto &boundary : boundaries->getBoundaries()) {
    SPDLOG_INFO("  - {} points, loop={}", boundary.getPoints().size(),
                boundary.isLoop());
  }
  if (boundarySimplificationType == 1) {
    if (boundaryMaxPoints.size() == 1) {
      boundaries->setMaxPoints(boundaryMaxPoints[0]);
    } else {
      // if boundary points not correctly specified use automatic values instead
      boundaries->setMaxPoints();
    }
  } else if (boundarySimplificationType == 0) {
    if (boundaryMaxPoints.size() != boundaries->size()) {
      // if boundary points not correctly specified use automatic values instead
      SPDLOG_INFO(
          "boundaryMaxPoints has size {}, but there are {} boundaries - "
          "using automatic values",
          boundaryMaxPoints.size(), boundaries->size());
      boundaries->setMaxPoints();
    } else {
      for (std::size_t i = 0; i < boundaryMaxPoints.size(); ++i) {
        boundaries->setMaxPoints(i, boundaryMaxPoints[i]);
      }
    }
  }
  SPDLOG_INFO("simplified {} boundaries", boundaries->size());
  for (const auto &boundary : boundaries->getBoundaries()) {
    SPDLOG_INFO("  - {} points, loop={}", boundary.getPoints().size(),
                boundary.isLoop());
  }
  if (compartmentMaxTriangleArea.size() != compartmentColors.size()) {
    // if triangle areas not correctly specified pick a value proportional to
    // the image size
    constexpr std::size_t minArea{1};
    constexpr std::size_t maxArea{999999};
    auto propArea =
        static_cast<std::size_t>(image.width() * image.height() / 250);
    auto area = std::clamp(propArea, minArea, maxArea);
    compartmentMaxTriangleArea =
        std::vector<std::size_t>(compartmentColors.size(), area);
    SPDLOG_INFO("no max triangle areas specified, using: {}", area);
  }
  constructMesh();
}

Mesh2d::~Mesh2d() = default;

bool Mesh2d::isValid() const { return validMesh; }

const std::string &Mesh2d::getErrorMessage() const { return errorMessage; };

std::size_t Mesh2d::getNumBoundaries() const { return boundaries->size(); }

std::size_t Mesh2d::getBoundarySimplificationType() const {
  return boundaries->getSimplifierType();
}

void Mesh2d::setBoundarySimplificationType(
    std::size_t boundarySimplificationType) {
  boundaries->setSimplifierType(boundarySimplificationType);
}

void Mesh2d::setBoundaryMaxPoints(std::size_t boundaryIndex,
                                  std::size_t maxPoints) {
  SPDLOG_INFO("boundaryIndex {}: max points {} -> {}", boundaryIndex,
              boundaries->getMaxPoints(boundaryIndex), maxPoints);
  boundaries->setMaxPoints(boundaryIndex, maxPoints);
}

std::size_t Mesh2d::getBoundaryMaxPoints(std::size_t boundaryIndex) const {
  return boundaries->getMaxPoints(boundaryIndex);
}

std::vector<std::size_t> Mesh2d::getBoundaryMaxPoints() const {
  return boundaries->getMaxPoints();
}

void Mesh2d::setCompartmentMaxTriangleArea(std::size_t compartmentIndex,
                                           std::size_t maxTriangleArea) {
  SPDLOG_INFO("compartmentIndex {}: max triangle area -> {}", compartmentIndex,
              maxTriangleArea);
  if (compartmentIndex >= compartmentMaxTriangleArea.size()) {
    SPDLOG_WARN(
        "compartmentIndex {} out of range for mesh with {} compartments",
        compartmentIndex, compartmentMaxTriangleArea.size());
    return;
  }
  auto &currentMaxTriangleArea =
      compartmentMaxTriangleArea.at(compartmentIndex);
  if (currentMaxTriangleArea == maxTriangleArea) {
    SPDLOG_INFO("  -> max triangle area unchanged");
    return;
  }
  currentMaxTriangleArea = maxTriangleArea;
  constructMesh();
}

std::size_t
Mesh2d::getCompartmentMaxTriangleArea(std::size_t compartmentIndex) const {
  return compartmentMaxTriangleArea.at(compartmentIndex);
}

const std::vector<std::size_t> &Mesh2d::getCompartmentMaxTriangleArea() const {
  return compartmentMaxTriangleArea;
}

const std::vector<std::vector<QPointF>> &
Mesh2d::getCompartmentInteriorPoints() const {
  return compartmentInteriorPoints;
}

void Mesh2d::setPhysicalGeometry(const common::VolumeF &voxelSize,
                                 const common::VoxelF &originPoint) {
  pixel = {voxelSize.width(), voxelSize.height()};
  origin = originPoint.p;
}

std::vector<double> Mesh2d::getVerticesAsFlatArray() const {
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
Mesh2d::getTriangleIndicesAsFlatArray(std::size_t compartmentIndex) const {
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
Mesh2d::getTriangleIndices() const {
  return triangleIndices;
}

void Mesh2d::constructMesh() {
  try {
    Triangulate triangulate(boundaries->getBoundaries(),
                            compartmentInteriorPoints,
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

static QPointF getScaleFactor(const QImage &img, const QSizeF &pixel,
                              const QSize &displaySize, const QPointF &offset) {
  auto displayWidth{static_cast<double>(displaySize.width()) - 2 * offset.x()};
  auto displayHeight{static_cast<double>(displaySize.height()) -
                     2 * offset.y()};
  double displayAspectRatio{displayWidth / displayHeight};
  double physicalAspectRatio{(img.width() * pixel.width()) /
                             (img.height() * pixel.height())};
  if (displayAspectRatio > physicalAspectRatio) {
    return {displayHeight * physicalAspectRatio /
                static_cast<double>(img.width()),
            displayHeight / static_cast<double>(img.height())};
  }
  return {
      displayWidth / static_cast<double>(img.width()),
      displayWidth / physicalAspectRatio / static_cast<double>(img.height()),
  };
}

std::pair<common::ImageStack, common::ImageStack>
Mesh2d::getBoundariesImages(const QSize &size,
                            std::size_t boldBoundaryIndex) const {
  constexpr int defaultPenSize = 2;
  constexpr int boldPenSize = 5;
  constexpr int maskPenSize = 15;
  QPointF offset(5.0, 5.0);
  auto scaleFactor{getScaleFactor(img, pixel, size, offset)};

  // construct boundary image
  common::ImageStack boundaryImage(
      {static_cast<int>(scaleFactor.x() * img.width() + 2 * offset.x()),
       static_cast<int>(scaleFactor.y() * img.height() + 2 * offset.y()), 1},
      QImage::Format_ARGB32_Premultiplied);
  boundaryImage.setVoxelSize({pixel.width(), pixel.height(), 1.0});
  boundaryImage.fill(qRgba(0, 0, 0, 0));

  common::ImageStack maskImage(boundaryImage.volume(),
                               QImage::Format_ARGB32_Premultiplied);
  maskImage.setVoxelSize({pixel.width(), pixel.height(), 1.0});
  maskImage.fill(qRgb(255, 255, 255));

  constexpr std::size_t zindex{0};
  QPainter painter(&boundaryImage[zindex]);
  painter.setRenderHint(QPainter::Antialiasing);

  QPainter pMask(&maskImage[zindex]);

  // draw boundary lines
  for (std::size_t k = 0; k < boundaries->size(); ++k) {
    const auto &points = boundaries->getBoundaries()[k].getPoints();
    int penSize = defaultPenSize;
    if (k == boldBoundaryIndex) {
      penSize = boldPenSize;
    }
    painter.setPen(QPen(common::indexedColors()[k], penSize));
    pMask.setPen(QPen(QColor(0, 0, static_cast<int>(k)), maskPenSize));
    for (std::size_t i = 0; i < points.size(); ++i) {
      QPointF p1(points[i].x() * scaleFactor.x() + offset.x(),
                 points[i].y() * scaleFactor.y() + offset.y());
      auto after_i{i < points.size() - 1 ? i + 1 : 0};
      QPointF p2(points[after_i].x() * scaleFactor.x() + offset.x(),
                 points[after_i].y() * scaleFactor.y() + offset.y());
      painter.drawEllipse(p1, penSize, penSize);
      if (i < points.size() - 1 || boundaries->getBoundaries()[k].isLoop()) {
        painter.drawLine(p1, p2);
        pMask.drawLine(p1, p2);
      }
    }
  }
  painter.end();
  pMask.end();
  // flip image on y-axis, to change (0,0) from bottom-left to top-left corner
  boundaryImage.flipYAxis();
  maskImage.flipYAxis();
  return std::make_pair(std::move(boundaryImage), std::move(maskImage));
}

std::pair<common::ImageStack, common::ImageStack>
Mesh2d::getMeshImages(const QSize &size, std::size_t compartmentIndex) const {
  std::pair<common::ImageStack, common::ImageStack> imgPair;
  auto &[meshImage, maskImage] = imgPair;
  QPointF offset(5.0, 5.0);
  auto scaleFactor{getScaleFactor(img, pixel, size, offset)};
  // construct mesh image
  meshImage = common::ImageStack(
      {static_cast<int>(scaleFactor.x() * img.width() + 2 * offset.x()),
       static_cast<int>(scaleFactor.y() * img.height() + 2 * offset.y()), 1},
      QImage::Format_ARGB32_Premultiplied);
  meshImage.setVoxelSize({pixel.width(), pixel.height(), 1.0});
  meshImage.fill(0);
  // construct mask image
  constexpr std::size_t zindex{0};
  QPainter p(&meshImage[zindex]);
  p.setRenderHint(QPainter::Antialiasing);
  maskImage = common::ImageStack(meshImage.volume(), QImage::Format_RGB32);
  maskImage.setVoxelSize({pixel.width(), pixel.height(), 1.0});
  maskImage.fill(qRgb(255, 255, 255));
  QPainter pMask(&maskImage[zindex]);
  // draw triangles
  for (std::size_t k = 0; k < triangles.size(); ++k) {
    if (k == compartmentIndex) {
      // fill triangles in chosen compartment & outline with bold lines
      p.setPen(QPen(Qt::black, 2));
      p.setBrush(QBrush(QColor(235, 235, 255)));
    } else {
      // outline all other triangles with gray lines
      p.setPen(QPen(Qt::gray, 1));
      p.setBrush(QColor(255, 255, 255, 0));
    }
    pMask.setBrush(QBrush(QColor(0, 0, static_cast<int>(k))));
    for (const auto &t : triangles[k]) {
      std::array<QPointF, 3> points;
      for (std::size_t i = 0; i < 3; ++i) {
        points[i].rx() = t[i].x() * scaleFactor.x() + offset.x();
        points[i].ry() = t[i].y() * scaleFactor.y() + offset.y();
      }
      p.drawConvexPolygon(points.data(), 3);
      pMask.drawConvexPolygon(points.data(), 3);
    }
  }
  pMask.end();
  // draw vertices
  p.setPen(QPen(Qt::red, 3));
  for (const auto &v : vertices) {
    p.drawPoint(QPointF(v.x() * scaleFactor.x() + offset.x(),
                        v.y() * scaleFactor.y() + offset.y()));
  }
  p.end();
  meshImage.flipYAxis();
  maskImage.flipYAxis();
  return imgPair;
}

QString Mesh2d::getGMSH() const {
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
                   .arg(common::dblToQStr(physicalPoint.x()))
                   .arg(common::dblToQStr(physicalPoint.y()))
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

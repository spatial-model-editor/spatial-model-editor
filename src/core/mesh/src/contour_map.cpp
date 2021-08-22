#include "contour_map.hpp"

namespace sme::mesh {

static void addEdge(ContourIndices &contourIndices, int contourIndex) {
  if (contourIndices[0] < 0 || contourIndices[0] == contourIndex) {
    contourIndices[0] = contourIndex;
    return;
  }
  if (contourIndices[1] < 0 || contourIndices[1] == contourIndex) {
    contourIndices[1] = contourIndex;
    return;
  }
  if (contourIndices[2] < 0 || contourIndices[2] == contourIndex) {
    contourIndices[2] = contourIndex;
    return;
  }
  contourIndices[3] = contourIndex;
}

static void addEdges(std::vector<ContourIndices> &indices,
                     const std::vector<cv::Point> &points, int contourIndex,
                     int L) {
  for (const auto &point : points) {
    auto idx{static_cast<std::size_t>(point.x + L * point.y)};
    addEdge(indices[idx], contourIndex);
  }
}

ContourMap::ContourMap(const QSize &size, const Contours &contours)
    : indices(
          static_cast<std::size_t>((size.height() + 1) * (size.width() + 1)),
          {-1, -1, -1, -1}),
      L(size.width() + 1) {
  // add domain edges, all with the same index
  auto domainIndex{static_cast<int>(contours.compartmentEdges.size())};
  for (const auto &edges : contours.domainEdges) {
    addEdges(indices, edges, domainIndex, L);
  }
  // add compartment edges
  int contourIndex{0};
  for (const auto &edges : contours.compartmentEdges) {
    addEdges(indices, edges, contourIndex, L);
    ++contourIndex;
  }
}

const ContourIndices &ContourMap::getContourIndices(const cv::Point &p) const {
  return indices[static_cast<std::size_t>(p.x + L * p.y)];
}

bool ContourMap::isFixedPoint(const cv::Point &p) const {
  return getContourIndices(p)[2] >= 0;
}

} // namespace sme::mesh

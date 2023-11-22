#include "triangulate.hpp"
#include "sme/logger.hpp"
#include "triangulate_common.hpp"
#include <CGAL/Constrained_Delaunay_triangulation_2.h>
#include <CGAL/Delaunay_mesh_face_base_2.h>
#include <CGAL/Delaunay_mesh_size_criteria_2.h>
#include <CGAL/Delaunay_mesher_2.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Triangulation_vertex_base_with_id_2.h>
#include <QPointF>
#include <algorithm>
#include <initializer_list>

using CGALKernel = CGAL::Exact_predicates_inexact_constructions_kernel;
using CGALVertex = CGAL::Triangulation_vertex_base_with_id_2<CGALKernel>;
using CGALFace = CGAL::Delaunay_mesh_face_base_2<CGALKernel>;
using CGALTds = CGAL::Triangulation_data_structure_2<CGALVertex, CGALFace>;
using CDT = CGAL::Constrained_Delaunay_triangulation_2<CGALKernel, CGALTds>;

namespace sme::mesh {

static TriangulateTriangleIndex toTriangleIndex(const CDT::Face_handle face) {
  return {static_cast<std::size_t>(face->vertex(0)->id()),
          static_cast<std::size_t>(face->vertex(1)->id()),
          static_cast<std::size_t>(face->vertex(2)->id())};
}

static void
addTriangle(const CDT::Face_handle face,
            std::vector<CDT::Face_handle> &facesToProcess,
            std::vector<TriangulateTriangleIndex> &triangleIndices) {
  face->set_marked(true);
  facesToProcess.push_back(face);
  triangleIndices.push_back(toTriangleIndex(face));
}

static void addTriangleAndNeighbours(
    CDT &cdt, const CDT::Face_handle startingFace,
    std::vector<TriangulateTriangleIndex> &triangleIndices) {
  std::vector<CDT::Face_handle> faces;
  faces.reserve(512);
  std::size_t faceIndex{0};
  addTriangle(startingFace, faces, triangleIndices);
  while (faceIndex < faces.size()) {
    auto face{faces[faceIndex]};
    ++faceIndex;
    for (int edgeIndex = 0; edgeIndex < 3; edgeIndex++) {
      // check each edge of this face (aka triangle)
      if (!face->is_constrained(edgeIndex)) {
        // edge is not a boundary, so look at connected face
        const auto connectedFace{face->neighbor(edgeIndex)};
        if (cdt.is_infinite(connectedFace)) {
          // if the edge is not constrained, but the neighbouring face is
          // infinite, we are outside of the boundary lines (since any infinite
          // face should be separated from the rest of the triangulation by a
          // constrained edge) which probably means the interior point was
          // outside of the appropriate region
          std::string msg{"Triangle is outside of the boundary lines"};
          SPDLOG_WARN("{}", msg);
          throw std::runtime_error(msg);
        }
        if (!connectedFace->is_marked()) {
          // connected face is valid and has not already been added, so add it
          addTriangle(connectedFace, faces, triangleIndices);
        }
      }
    }
  }
}

static std::vector<TriangulateTriangleIndex>
getConnectedTriangleIndices(CDT &cdt,
                            const std::vector<QPointF> &interiorPoints) {
  std::vector<TriangulateTriangleIndex> triangleIndices;
  for (auto face = cdt.all_faces_begin(); face != cdt.all_faces_end(); ++face) {
    face->set_marked(false);
  }
  for (const auto &interiorPoint : interiorPoints) {
    SPDLOG_INFO("Adding interior point ({},{})", interiorPoint.x(),
                interiorPoint.y());
    SPDLOG_INFO("  - triangles before: {}", triangleIndices.size());
    if (auto face{cdt.locate(CDT::Point(interiorPoint.x(), interiorPoint.y()))};
        face != nullptr && !face->is_marked()) {
      addTriangleAndNeighbours(cdt, face, triangleIndices);
    }
    SPDLOG_INFO("  - triangles after: {}", triangleIndices.size());
  }
  return triangleIndices;
}

static CDT makeConstrainedDelaunayTriangulation(
    const TriangulateBoundaries &triangulateBoundaries) {
  CDT cdt;
  std::vector<CDT::Vertex_handle> vertices;
  vertices.reserve(triangulateBoundaries.vertices.size());
  for (const auto &p : triangulateBoundaries.vertices) {
    // handle == pointer to the vertex
    auto handle = cdt.insert(CDT::Point(p.x(), p.y()));
    vertices.push_back(handle);
  }
  for (const auto &boundary : triangulateBoundaries.boundaries) {
    for (const auto &seg : boundary) {
      // constraint == planar line boundary segment between these two vertices
      cdt.insert_constraint(vertices[seg.start], vertices[seg.end]);
    }
  }
  SPDLOG_INFO("Number of vertices in CDT: {}", cdt.number_of_vertices());
  SPDLOG_INFO("Number of triangles in CDT: {}", cdt.number_of_faces());
  return cdt;
}

static void meshCdt(CDT &cdt,
                    const std::vector<TriangulateCompartment> &compartments) {
  // compartments are meshed in ascending order of max triangle area, to avoid
  // steiner points being added to a boundary of a coarsely meshed compartment
  // resulting in the insertion of bad (tall/thin) triangles in the already
  // meshed compartment.
  auto sortedCompartments{compartments};
  std::sort(sortedCompartments.begin(), sortedCompartments.end(),
            [](const auto &a, const auto &b) {
              return a.maxTriangleArea < b.maxTriangleArea;
            });
  for (const auto &compartment : sortedCompartments) {
    std::vector<CDT::Point> seeds;
    seeds.reserve(compartment.interiorPoints.size());
    for (const auto &ip : compartment.interiorPoints) {
      seeds.emplace_back(ip.x(), ip.y());
    }
    double maxArea{compartment.maxTriangleArea};
    // convert max area constraint to a max triangle edge length constraint
    // assume equilateral triangles, so area = sqrt(3) length^2 / 4
    double maxLength{1.5196713713 * std::sqrt(maxArea)};
    SPDLOG_INFO("Max area {} -> max length {}", maxArea, maxLength);
    // https://doc.cgal.org/latest/Mesh_2/classCGAL_1_1Delaunay__mesh__size__criteria__2.html
    constexpr double bestAngleBoundWithGuaranteedTermination{0.125};
    CGAL::Delaunay_mesh_size_criteria_2<CDT> criteria(
        bestAngleBoundWithGuaranteedTermination, maxLength);
    CGAL::refine_Delaunay_mesh_2(
        cdt,
        CGAL::parameters::seeds(seeds).criteria(criteria).seeds_are_in_domain(
            true));
    SPDLOG_INFO("Number of vertices in mesh: {}", cdt.number_of_vertices());
    SPDLOG_INFO("Number of triangles in mesh: {}", cdt.number_of_faces());
  }
}

static std::vector<QPointF> getPointsFromCdt(CDT &cdt) {
  std::vector<QPointF> points;
  points.reserve(cdt.number_of_vertices());
  int ih{0};
  for (auto vertex = cdt.finite_vertices_begin();
       vertex != cdt.finite_vertices_end(); ++vertex) {
    points.emplace_back(vertex->point().x(), vertex->point().y());
    // set id() of a vertex to its index in the points vector
    vertex->id() = ih;
    ++ih;
  }
  SPDLOG_INFO("Mesh has {} vertices", points.size());
  return points;
}

Triangulate::Triangulate(
    const std::vector<Boundary> &boundaries,
    const std::vector<std::vector<QPointF>> &interiorPoints,
    const std::vector<std::size_t> &maxTriangleAreas) {
  TriangulateBoundaries tb(boundaries, interiorPoints, maxTriangleAreas);
  auto cdt{makeConstrainedDelaunayTriangulation(tb)};
  meshCdt(cdt, tb.compartments);
  points = getPointsFromCdt(cdt);
  for (const auto &compartment : tb.compartments) {
    triangleIndices.push_back(
        getConnectedTriangleIndices(cdt, compartment.interiorPoints));
    SPDLOG_INFO("added compartment with {} triangles",
                triangleIndices.back().size());
  }
}

const std::vector<QPointF> &Triangulate::getPoints() const { return points; }

const std::vector<std::vector<TriangulateTriangleIndex>> &
Triangulate::getTriangleIndices() const {
  return triangleIndices;
}

} // namespace sme::mesh

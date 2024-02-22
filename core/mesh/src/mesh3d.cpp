#include "sme/mesh3d.hpp"
#include "sme/logger.hpp"
#include "sme/utils.hpp"
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

// https://doc.cgal.org/latest/Mesh_3/Mesh_3_2mesh_3D_image_8cpp-example.html
#include "CGAL_Regular_triangulation_vertex_base_3_with_id.hpp"
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/ImageIO.h>
#include <CGAL/Image_3.h>
#include <CGAL/Labeled_mesh_domain_3.h>
#include <CGAL/Mesh_complex_3_in_triangulation_3.h>
#include <CGAL/Mesh_criteria_3.h>
#include <CGAL/Mesh_triangulation_3.h>
#include <CGAL/Triangulation_vertex_base_with_info_3.h>
#include <CGAL/make_mesh_3.h>
using CGALKernel = CGAL::Exact_predicates_inexact_constructions_kernel;
using CGALMeshDomain = CGAL::Labeled_mesh_domain_3<CGALKernel>;
// use a custom vertex type with an id for indexing
using CGALVertex =
    sme::mesh::CGAL_Regular_triangulation_vertex_base_3_with_id<CGALKernel>;
// need some non-defaulted types to be able to specify this custom vertex base
using CGALGeomTraits =
    CGAL::details::Mesh_geom_traits_generator<CGALKernel>::type;
using CGALIndicesTuple =
    CGAL::Mesh_3::internal::Indices_tuple_t<CGALMeshDomain>;
using CGALIndex = CGALMeshDomain::Index;
using CGALMeshVertex =
    CGAL::Mesh_vertex_generator_3<CGALGeomTraits, CGALIndicesTuple, CGALIndex,
                                  CGALVertex>;
using CGALTriangulation =
    CGAL::Mesh_triangulation_3<CGALMeshDomain, CGALKernel, CGAL::Sequential_tag,
                               CGALMeshVertex>::type;
using CGALC3t3 = CGAL::Mesh_complex_3_in_triangulation_3<CGALTriangulation>;
using CGALMeshCriteria = CGAL::Mesh_criteria_3<CGALTriangulation>;
using CGALSizingField =
    CGAL::Mesh_constant_domain_field_3<CGALKernel, CGALIndex>;

namespace sme::mesh {

static sme::common::VoxelF toVoxelF(CGALC3t3::Vertex_handle vertexHandle) {
  return {vertexHandle->point().x(), vertexHandle->point().y(),
          vertexHandle->point().z()};
}

static constexpr auto NullCompartmentIndex =
    std::numeric_limits<std::uint8_t>::max();

void Mesh3d::constructMesh() {
  vertices_.clear();
  for (auto &v : tetrahedronVertexIndices_) {
    v.clear();
  }
  for (auto &v : tetrahedra_) {
    v.clear();
  }

  auto domain = CGALMeshDomain::create_labeled_image_mesh_domain(
      *image3_, CGAL::parameters::default_values().value_outside(0));
  SPDLOG_INFO("Constructed mesh domain from Image_3");

  // radius of a sphere with approx same volume of voxel (using smallest side of
  // voxel if anisotropic)
  double approxVoxelRadius =
      0.6 * (std::min(image3_->vx(), std::min(image3_->vy(), image3_->vz())));
  // CGAL cell size is upper-bound for the circumradii of the mesh tetrahedra
  // see https://doc.cgal.org/5.1/Mesh_3/classCGAL_1_1Mesh__criteria__3.html
  double maxCellSize =
      static_cast<double>(common::max(compartmentMaxCellVolume_)) *
      approxVoxelRadius;
  SPDLOG_INFO("Max cell size: {}", maxCellSize);
  CGALSizingField cell_size_field(maxCellSize);
  for (std::size_t compIndex = 0; compIndex < compartmentMaxCellVolume_.size();
       ++compIndex) {
    auto subdomainIndex =
        common::element_index(labelToCompartmentIndex_, compIndex);
    double cellSize =
        static_cast<double>(compartmentMaxCellVolume_[compIndex]) *
        approxVoxelRadius;
    cell_size_field.set_size(
        cellSize, dim,
        domain.index_from_subdomain_index(static_cast<int>(subdomainIndex)));
    SPDLOG_INFO("  - subdomain {} max cell size: {}", subdomainIndex, cellSize);
  }
  // todo: expose more criteria to user, such as facet_size or facet_distance
  CGALMeshCriteria criteria(CGAL::parameters::facet_angle(25),
                            CGAL::parameters::facet_size(maxCellSize),
                            CGAL::parameters::facet_distance(maxCellSize),
                            CGAL::parameters::cell_radius_edge_ratio(4.0),
                            CGAL::parameters::cell_size(cell_size_field));

  // todo: this can hang or segfault for difficult / badly formed inputs, need
  // to sanitize inputs
  auto c3t3 = CGAL::make_mesh_3<CGALC3t3>(domain, criteria);
  validMesh_ = c3t3.is_valid();
  if (!validMesh_) {
    SPDLOG_ERROR("Invalid mesh generated");
    return;
  }
  SPDLOG_INFO("Constructed mesh from domain and criteria");
  SPDLOG_INFO("  - num vertices: {}",
              c3t3.triangulation().number_of_vertices());
  SPDLOG_INFO("  - num cells: {}", c3t3.triangulation().number_of_cells());

  // add an id to each index (index of this vertex in our vertices_ vector)
  for (int id{0}; auto v : c3t3.triangulation().finite_vertex_handles()) {
    v->id() = id;
    SPDLOG_TRACE("vertex {} at {},{},{}", v->id(), v->point().x(),
                 v->point().y(), v->point().z());
    vertices_.emplace_back(v->point().x(), v->point().y(), v->point().z());
    ++id;
  }

  for (auto c : c3t3.triangulation().finite_cell_handles()) {
    if (auto compartmentIndex =
            labelToCompartmentIndex_[c3t3.subdomain_index(c)];
        compartmentIndex != NullCompartmentIndex) {
      SPDLOG_TRACE(
          "cell with vertex ids {},{},{},{} in subdomain {} -> compartment {}",
          c->vertex(0)->id(), c->vertex(1)->id(), c->vertex(2)->id(),
          c->vertex(3)->id(), c3t3.subdomain_index(c), compartmentIndex);
      auto &vi = tetrahedronVertexIndices_[compartmentIndex].emplace_back();
      for (int i = 0; i < 4; ++i) {
        vi[i] = static_cast<std::size_t>(c->vertex(i)->id());
      }
      auto &t = tetrahedra_[compartmentIndex].emplace_back();
      for (int i = 0; i < 4; ++i) {
        t[i] = toVoxelF(c->vertex(i));
      }
    }
  }
}

Mesh3d::Mesh3d() = default;

Mesh3d::Mesh3d(const sme::common::ImageStack &imageStack,
               std::vector<std::size_t> maxCellVolume,
               const common::VolumeF &voxelSize,
               const common::VoxelF &originPoint,
               const std::vector<QRgb> &compartmentColours)
    : compartmentMaxCellVolume_(std::move(maxCellVolume)) {
  if (imageStack.empty()) {
    errorMessage_ = "Compartment geometry image missing";
    return;
  }
  // ensure we are using an indexed image3 stack
  auto imageStack_ = imageStack;
  imageStack_.convertToIndexed();
  auto vol = imageStack_.volume();
  const auto &colorTable = imageStack_[0].colorTable();
  SPDLOG_INFO("ImageStack {}x{}x{} with {} colours", vol.width(), vol.height(),
              vol.depth(), colorTable.size());

  for (auto compartmentColor : compartmentColours) {
    if (!colorTable.contains(compartmentColor)) {
      SPDLOG_WARN("Compartment color is not present in geometry image");
      errorMessage_ = "Compartment color is not present in geometry image";
      return;
    }
  }

  // convert from label to compartment index: label 0 is reserved for out of
  // mesh voxels, so label n is colorIndex n-1 from the image colorTable
  labelToCompartmentIndex_.assign(colorTable.size() + 1, NullCompartmentIndex);
  for (std::size_t compartmentIndex = 0;
       compartmentIndex < compartmentColours.size(); ++compartmentIndex) {
    auto compartmentColor = compartmentColours[compartmentIndex];
    auto colorIndex = colorTable.indexOf(compartmentColor);
    if (colorIndex >= 0) {
      auto label = colorIndex + 1;
      labelToCompartmentIndex_[label] =
          static_cast<std::uint8_t>(compartmentIndex);
      SPDLOG_INFO("  - label {} -> compartment {} / color {:x}", label,
                  compartmentIndex, compartmentColor);
    }
  }

  if (std::ranges::all_of(labelToCompartmentIndex_, [](std::uint8_t a) {
        return a == NullCompartmentIndex;
      })) {
    SPDLOG_WARN("No compartments have been assigned a colour: meshing aborted");
    errorMessage_ = "No compartments have been assigned a colour";
    return;
  }

  // convert image3 stack to CGAL Image_3 with label = colorIndex + 1 for each
  // voxel, as label 0 is reserved for voxels that lie outside of the mesh
  auto *pointImage =
      _createImage(vol.width(), vol.height(), vol.depth(), 1, 1, 1, 1, 1,
                   WORD_KIND::WK_FIXED, SIGN::SGN_UNSIGNED);
  auto *ptr = static_cast<std::uint8_t *>(pointImage->data);
  for (std::size_t z = 0; z < vol.depth(); ++z) {
    for (int y = vol.height() - 1; y >= 0; --y) {
      for (int x = 0; x < vol.width(); ++x) {
        auto label =
            static_cast<std::uint8_t>(imageStack_[z].pixelIndex(x, y) + 1);
        *ptr = label;
        ptr++;
      }
    }
  }
  image3_ = std::make_unique<CGAL::Image_3>(pointImage);
  SPDLOG_INFO("Constructed {}x{}x{} Image_3", image3_->xdim(), image3_->ydim(),
              image3_->zdim());

  tetrahedronVertexIndices_.resize(compartmentColours.size());
  tetrahedra_.resize(compartmentColours.size());

  if (compartmentMaxCellVolume_.size() != compartmentColours.size()) {
    // if cell volumes are not correctly specified use default value
    constexpr std::size_t defaultCompartmentMaxCellVolume{5};
    compartmentMaxCellVolume_ = std::vector<std::size_t>(
        compartmentColours.size(), defaultCompartmentMaxCellVolume);
    SPDLOG_INFO("no max cell volumes specified, using default value: {}",
                defaultCompartmentMaxCellVolume);
  }
  setPhysicalGeometry(voxelSize, originPoint);
}

Mesh3d::~Mesh3d() = default;

bool Mesh3d::isValid() const { return validMesh_; }

const std::string &Mesh3d::getErrorMessage() const { return errorMessage_; };

void Mesh3d::setCompartmentMaxCellVolume(std::size_t compartmentIndex,
                                         std::size_t maxCellVolume) {
  SPDLOG_INFO("compIndex {}: max cell volume {} -> {}", compartmentIndex,
              compartmentMaxCellVolume_.at(compartmentIndex), maxCellVolume);
  compartmentMaxCellVolume_.at(compartmentIndex) = maxCellVolume;
  constructMesh();
}

std::size_t
Mesh3d::getCompartmentMaxCellVolume(std::size_t compartmentIndex) const {
  return compartmentMaxCellVolume_.at(compartmentIndex);
}

const std::vector<std::size_t> &Mesh3d::getCompartmentMaxCellVolume() const {
  return compartmentMaxCellVolume_;
}

void Mesh3d::setPhysicalGeometry(const common::VolumeF &voxelSize,
                                 const common::VoxelF &originPoint) {
  image3_->image()->vx = voxelSize.width();
  image3_->image()->vy = voxelSize.height();
  image3_->image()->vz = voxelSize.depth();
  image3_->image()->tx = static_cast<float>(originPoint.p.x());
  image3_->image()->ty = static_cast<float>(originPoint.p.y());
  image3_->image()->tz = static_cast<float>(originPoint.z);
  SPDLOG_INFO("{}x{}x{} Image_3 of {}x{}x{} voxels with origin ({},{},{})",
              image3_->xdim(), image3_->ydim(), image3_->zdim(), image3_->vx(),
              image3_->vy(), image3_->vz(), image3_->tx(), image3_->ty(),
              image3_->tz());
  constructMesh();
}

std::vector<double> Mesh3d::getVerticesAsFlatArray() const {
  std::vector<double> v;
  v.reserve(vertices_.size() * 3);
  for (const auto &vertex : vertices_) {
    v.push_back(vertex.p.x());
    v.push_back(vertex.p.y());
    v.push_back(vertex.z);
  }
  return v;
}

std::vector<QVector4D>
Mesh3d::getVerticesAsQVector4DArrayInHomogeneousCoord() const {
  // similar with getVerticesAsFlatArray() but in Homogeneous Coordinates
  std::vector<QVector4D> v;
  v.reserve(vertices_.size());
  for (const auto &vertex : vertices_) {
    v.push_back(QVector4D(static_cast<float>(vertex.p.x()),
                          static_cast<float>(vertex.p.y()),
                          static_cast<float>(vertex.z), 1.0f));
  }
  return v;
}

std::size_t Mesh3d::getNumberOfCompartment() const {
  return tetrahedronVertexIndices_.size();
}

std::vector<int>
Mesh3d::getTetrahedronIndicesAsFlatArray(std::size_t compartmentIndex) const {

  assert(compartmentIndex < tetrahedronVertexIndices_.size());

  std::vector<int> out;
  const auto &indices = tetrahedronVertexIndices_[compartmentIndex];
  out.reserve(indices.size() * 4);
  for (const auto &t : indices) {
    for (std::size_t ti : t) {
      out.push_back(static_cast<int>(ti));
    }
  }
  return out;
}

std::vector<uint32_t>
Mesh3d::getMeshSegmentsIndicesAsFlatArray(std::size_t compartmentIndex) const {
  assert(compartmentIndex < tetrahedronVertexIndices_.size());

  std::vector<uint32_t> out;
  const auto &indices = tetrahedronVertexIndices_[compartmentIndex];
  out.reserve(indices.size() * 6);
  for (const auto &t : indices) {
    for (uint32_t i = 0; i < t.size() - 1; i++) {
      for (uint32_t j = i + 1; j < t.size(); j++) {
        out.push_back(static_cast<uint32_t>(t[i]));
        out.push_back(static_cast<uint32_t>(t[j]));
      }
    }
  }
  return out;
}

const std::vector<std::vector<TetrahedronVertexIndices>> &
Mesh3d::getTetrahedronIndices() const {
  return tetrahedronVertexIndices_;
}

QString Mesh3d::getGMSH() const {
  // note: gmsh indexing starts with 1, so we need to add 1 to all indices
  // meshing is done in terms of voxel geometry, to convert to physical points:
  //   - rescale each vertex by a factor pixel
  //   - add origin to each vertex
  QString msh;
  msh.append("$MeshFormat\n");
  msh.append("2.2 0 8\n");
  msh.append("$EndMeshFormat\n");
  msh.append("$Nodes\n");
  msh.append(QString("%1\n").arg(vertices_.size()));
  std::size_t nodeIndex = 1;
  for (const auto &v : vertices_) {
    msh.append(QString("%1 %2 %3 %4\n")
                   .arg(QString::number(nodeIndex), common::dblToQStr(v.p.x()),
                        common::dblToQStr(v.p.y()), common::dblToQStr(v.z)));
    ++nodeIndex;
  }
  msh.append("$EndNodes\n");
  msh.append("$Elements\n");
  std::size_t nElem = 0;
  std::size_t compartmentIndex = 1;
  for (const auto &comp : tetrahedronVertexIndices_) {
    SPDLOG_TRACE("Adding compartment of tetrahedra, index: {}",
                 compartmentIndex);
    nElem += comp.size();
    compartmentIndex++;
  }
  msh.append(QString("%1\n").arg(nElem));
  std::size_t elementIndex = 1;
  compartmentIndex = 1;
  for (const auto &comp : tetrahedronVertexIndices_) {
    SPDLOG_TRACE("Writing tetrahedra for compartment index: {}",
                 compartmentIndex);
    for (const auto &t : comp) {
      msh.append(QString("%1 4 2 %2 %2 %3 %4 %5 %6\n")
                     .arg(elementIndex)
                     .arg(compartmentIndex)
                     .arg(t[0] + 1)
                     .arg(t[1] + 1)
                     .arg(t[2] + 1)
                     .arg(t[3] + 1));
      ++elementIndex;
    }
    ++compartmentIndex;
  }
  msh.append("$EndElements\n");
  return msh;
}

} // namespace sme::mesh

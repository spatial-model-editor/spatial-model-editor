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

namespace sme::mesh {

static sme::common::VoxelF toVoxelF(CGALC3t3::Vertex_handle vertexHandle) {
  return {vertexHandle->point().x(), vertexHandle->point().y(),
          vertexHandle->point().z()};
}

sme::common::VoxelF Mesh3d::voxelPointToPhysicalPoint(
    const sme::common::VoxelF &voxel) const noexcept {
  common::VoxelF physicalPoint{originPoint_};
  physicalPoint.p.rx() += voxelSize_.width() * (voxel.p.x() + 0.5);
  physicalPoint.p.ry() += voxelSize_.height() * (voxel.p.y() + 0.5);
  physicalPoint.z += voxelSize_.depth() * (voxel.z + 0.5);
  return physicalPoint;
}

void Mesh3d::constructMesh() {
  for (auto &v : tetrahedronVertexIndices_) {
    v.clear();
  }
  for (auto &v : tetrahedra_) {
    v.clear();
  }

  // todo: presumably we can store the generated domain instead of regenerating
  // this?
  auto domain = CGALMeshDomain::create_labeled_image_mesh_domain(
      *image3_,
      CGAL::parameters::default_values().iso_value(0).value_outside(0));

  SPDLOG_INFO("Constructed mesh domain from Image_3");

  // todo: these have a large effect on the mesh results and at least some of
  // them need to be user adjustable and possibly also have some sanity checks
  // on them to ensure the meshing will work

  // todo: set cell size for each subdomain - for now just justing the same
  // value for all
  CGALMeshCriteria criteria(CGAL::parameters::facet_angle(30)
                                .facet_size(2)
                                .facet_distance(2)
                                .cell_radius_edge_ratio(3)
                                .cell_size(compartmentMaxCellVolume_[0]));

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
    if (c3t3.subdomain_index(c) > 0) {
      // we skip subdomain 0 as it is the "outside" / not a compartment
      auto compartmentIndex = c3t3.subdomain_index(c) - 1;
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
    : originPoint_(originPoint), voxelSize_(voxelSize),
      compartmentMaxCellVolume_(std::move(maxCellVolume)) {
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

  // todo: add check for > 255 colours in image3, as the meshing expects to get
  // a uchar for the labels

  // generate a label for each colour in the image3 stack, where label 0 means
  // no compartment, and label n means element n-1 from compartmentColour vector
  std::vector<std::uint8_t> colourIndexToLabel(colorTable.size(), 0);
  constexpr std::size_t notFound{std::numeric_limits<std::size_t>::max()};
  for (std::size_t i = 0; i < colourIndexToLabel.size(); ++i) {
    auto compartmentColourIndex =
        sme::common::element_index(compartmentColours, colorTable[i], notFound);
    if (compartmentColourIndex != notFound) {
      colourIndexToLabel[i] = compartmentColourIndex + 1;
    }
    SPDLOG_INFO("  - {:x} -> label {}", colorTable[i], colourIndexToLabel[i]);
  }

  if (std::ranges::all_of(colourIndexToLabel,
                          [](std::uint8_t a) { return a == 0; })) {
    SPDLOG_WARN("No compartments have been assigned a colour: meshing aborted");
    errorMessage_ = "No compartments have been assigned a colour";
    return;
  }

  // convert image3 stack to CGAL Image_3 with the label for each voxel
  auto *pointImage =
      _createImage(vol.width(), vol.height(), vol.depth(), 1, 1, 1, 1, 1,
                   WORD_KIND::WK_FIXED, SIGN::SGN_UNSIGNED);
  auto *ptr = static_cast<std::uint8_t *>(pointImage->data);
  for (std::size_t z = 0; z < vol.depth(); ++z) {
    for (int y = 0; y < vol.height(); ++y) {
      for (int x = 0; x < vol.width(); ++x) {
        auto label = colourIndexToLabel[imageStack_[z].pixelIndex(x, y)];
        *ptr = label;
        ptr++;
      }
    }
  }
  image3_ = std::make_unique<CGAL::Image_3>(pointImage);
  SPDLOG_INFO("Constructed {}x{}x{} Image_3 of {}x{}x{} voxels",
              image3_->xdim(), image3_->ydim(), image3_->zdim(), image3_->vx(),
              image3_->vy(), image3_->vz());

  tetrahedronVertexIndices_.resize(compartmentColours.size());
  tetrahedra_.resize(compartmentColours.size());

  if (compartmentMaxCellVolume_.size() != compartmentColours.size()) {
    // if cell volumes are not correctly specified use default value
    constexpr std::size_t defaultCompartmentMaxCellVolume{3};
    compartmentMaxCellVolume_ = std::vector<std::size_t>(
        compartmentColours.size(), defaultCompartmentMaxCellVolume);
    SPDLOG_INFO("no max cell volumes specified, using default value: {}",
                defaultCompartmentMaxCellVolume);
  }

  constructMesh();
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
  voxelSize_ = voxelSize;
  originPoint_ = originPoint;
}

std::vector<double> Mesh3d::getVerticesAsFlatArray() const {
  // convert from voxel to physical coordinates
  std::vector<double> v;
  v.reserve(vertices_.size() * 3);
  for (const auto &voxelPoint : vertices_) {
    auto physicalPoint = voxelPointToPhysicalPoint(voxelPoint);
    v.push_back(physicalPoint.p.x());
    v.push_back(physicalPoint.p.y());
    v.push_back(physicalPoint.z);
  }
  return v;
}

std::vector<QVector4D>
Mesh3d::getVerticesAsQVector4DArrayInHomogeneousCoord() const {
  // similar with getVerticesAsFlatArray() but in Homogeneous Coordinates
  std::vector<QVector4D> v;
  v.reserve(vertices_.size());
  for (const auto &voxelPoint : vertices_) {
    auto physicalPoint = voxelPointToPhysicalPoint(voxelPoint);
    v.push_back(QVector4D(physicalPoint.p.x(), physicalPoint.p.y(),
                          physicalPoint.z, 1.0f));
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
        out.push_back(t[i]);
        out.push_back(t[j]);
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
  for (std::size_t i = 0; i < vertices_.size(); ++i) {
    auto physicalPoint = voxelPointToPhysicalPoint(vertices_[i]);
    msh.append(QString("%1 %2 %3 %4\n")
                   .arg(i + 1)
                   .arg(common::dblToQStr(physicalPoint.p.x()))
                   .arg(common::dblToQStr(physicalPoint.p.y()))
                   .arg(common::dblToQStr(physicalPoint.z)));
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
  std::size_t elementIndex{1};
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

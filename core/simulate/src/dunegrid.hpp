//  - makeDuneGrid(): create dune-copasi grid from mesh
//  - based on MultiDomainGmshReader from dune-copasi
//  - Note: ensure any changes to MultiDomainGmshReader in future versions of
//  dune-copasi are taken into account here if relevant

#pragma once

#include "sme/mesh.hpp"
#include "sme/mesh3d.hpp"
#include <array>
#include <cstddef>
#include <dune/geometry/type.hh>
#include <dune/grid/common/gridfactory.hh>
#include <dune/grid/multidomaingrid/multidomaingrid.hh>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

namespace sme::simulate {

namespace detail {

static constexpr std::size_t UnusedVertexIndex =
    std::numeric_limits<unsigned int>::max();

static inline bool
useCompartment(std::size_t compIndex,
               const std::unordered_set<int> &compartmentIndicies) {
  return compartmentIndicies.empty() ||
         compartmentIndicies.find(static_cast<int>(compIndex)) !=
             compartmentIndicies.cend();
}

static inline unsigned int
getOrCreateVertexIndex(std::size_t oldVertexIndex,
                       std::vector<unsigned int> &newVertexIndices,
                       int &newVertexCount) {
  auto &newVertexIndex = newVertexIndices[oldVertexIndex];
  if (newVertexIndex == UnusedVertexIndex) {
    newVertexIndex = newVertexCount;
    ++newVertexCount;
  }
  return newVertexIndex;
}

static const std::vector<std::vector<mesh::TriangulateTriangleIndex>> &
getElementIndices(const mesh::Mesh &mesh) {
  return mesh.getTriangleIndices();
}

static const std::vector<std::vector<mesh::TetrahedronVertexIndices>> &
getElementIndices(const mesh::Mesh3d &mesh3d) {
  return mesh3d.getTetrahedronIndices();
}

template <typename HostGrid, typename MeshType>
std::pair<std::vector<std::size_t>, std::shared_ptr<HostGrid>>
makeHostGrid(const MeshType &mesh) {
  Dune::GridFactory<HostGrid> factory;
  // get original vertices
  auto v = mesh.getVerticesAsFlatArray();
  // map old to new vertex index
  std::vector<unsigned int> newVertexIndices(v.size() / MeshType::dim,
                                             UnusedVertexIndex);
  int newVertexCount{0};
  // add elements from each compartment
  std::vector<std::size_t> numElements;
  std::vector<unsigned int> elementVertexIndices(MeshType::dim + 1, 0);
  for (const auto &elements : getElementIndices(mesh)) {
    numElements.push_back(elements.size());
    for (const auto &element : elements) {
      for (std::size_t iDim = 0; iDim <= MeshType::dim; ++iDim) {
        elementVertexIndices[iDim] = getOrCreateVertexIndex(
            element[iDim], newVertexIndices, newVertexCount);
      }
      factory.insertElement(Dune::GeometryTypes::simplex(MeshType::dim),
                            elementVertexIndices);
    }
  }
  // add vertices
  std::vector<double> newFlatVertices(
      MeshType::dim * static_cast<std::size_t>(newVertexCount), 0.0);
  for (std::size_t iOld = 0; iOld < newVertexIndices.size(); ++iOld) {
    if (auto iNew = newVertexIndices[iOld]; iNew != UnusedVertexIndex) {
      for (std::size_t iDim = 0; iDim < MeshType::dim; ++iDim) {
        newFlatVertices[MeshType::dim * iNew + iDim] =
            v[MeshType::dim * iOld + iDim];
      }
    }
  }
  Dune::FieldVector<typename HostGrid::ctype, MeshType::dim> fv;
  for (std::size_t i = 0; i < newFlatVertices.size() / MeshType::dim; ++i) {
    for (std::size_t iDim = 0; iDim < MeshType::dim; ++iDim) {
      fv[iDim] = newFlatVertices[MeshType::dim * i + iDim];
    }
    factory.insertVertex(fv);
  }
  return {numElements, factory.createGrid()};
}

template <typename HostGrid, typename MDGTraits>
auto makeGrid(HostGrid &hostGrid, std::size_t maxSubdomains) {
  using Grid = Dune::mdgrid::MultiDomainGrid<HostGrid, MDGTraits>;
  std::unique_ptr<MDGTraits> traits;
  if constexpr (std::is_default_constructible_v<MDGTraits>) {
    traits = std::make_unique<MDGTraits>();
  } else {
    traits = std::make_unique<MDGTraits>(maxSubdomains);
  }
  return std::make_shared<Grid>(hostGrid, *traits);
}

template <typename Grid>
void assignElements(Grid &grid, const std::vector<std::size_t> &numElements) {
  // assign each element to its subdomain / compartment
  std::size_t iCompartment{0};
  std::size_t iElement{0};
  grid->startSubDomainMarking();
  for (const auto &cell : elements(grid->leafGridView())) {
    while (iElement == numElements[iCompartment]) {
      iElement = 0;
      ++iCompartment;
    }
    grid->addToSubDomain(static_cast<int>(iCompartment), cell);
    ++iElement;
  }
  grid->preUpdateSubDomains();
  grid->updateSubDomains();
  grid->postUpdateSubDomains();
}

} // namespace detail

template <class HostGrid, class MDGTraits, class MeshType>
auto makeDuneGrid(const MeshType &mesh) {
  auto [numElements, hostGrid] = detail::makeHostGrid<HostGrid>(mesh);
  auto grid =
      detail::makeGrid<HostGrid, MDGTraits>(*hostGrid, numElements.size());
  detail::assignElements(grid, numElements);
  return std::make_pair(grid, hostGrid);
}

} // namespace sme::simulate

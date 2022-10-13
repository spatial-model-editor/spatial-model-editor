//  - makeDuneGrid(): create dune-copasi grid from mesh
//  - based on MultiDomainGmshReader from dune-copasi
//  - Note: ensure any changes to MultiDomainGmshReader in future versions of
//  dune-copasi are taken into account here if relevant

#pragma once

#include "sme/mesh.hpp"
#include <array>
#include <cstddef>
#include <dune/geometry/type.hh>
#include <dune/grid/common/gridfactory.hh>
#include <dune/grid/multidomaingrid/multidomaingrid.hh>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

namespace sme {

namespace simulate {

namespace detail {

static inline bool
useCompartment(std::size_t compIndex,
               const std::unordered_set<int> &compartmentIndicies) {
  return compartmentIndicies.empty() ||
         compartmentIndicies.find(static_cast<int>(compIndex)) !=
             compartmentIndicies.cend();
}

static inline unsigned int
getOrCreateVertexIndex(std::size_t oldVertexIndex,
                       std::vector<int> &newVertexIndices,
                       int &newVertexCount) {
  auto &newVertexIndex = newVertexIndices[oldVertexIndex];
  if (newVertexIndex < 0) {
    newVertexIndex = newVertexCount;
    ++newVertexCount;
  }
  return static_cast<unsigned int>(newVertexIndex);
}

template <typename HostGrid>
void insertTriangle(const std::array<std::size_t, 3> &t,
                    Dune::GridFactory<HostGrid> &factory,
                    std::vector<int> &newVertexIndices, int &newVertexCount) {
  factory.insertElement(
      Dune::GeometryTypes::triangle,
      {getOrCreateVertexIndex(t[0], newVertexIndices, newVertexCount),
       getOrCreateVertexIndex(t[1], newVertexIndices, newVertexCount),
       getOrCreateVertexIndex(t[2], newVertexIndices, newVertexCount)});
}

template <typename HostGrid>
std::pair<std::vector<std::size_t>, std::shared_ptr<HostGrid>>
makeHostGrid(const mesh::Mesh &mesh) {
  Dune::GridFactory<HostGrid> factory;
  // get original vertices
  auto v = mesh.getVerticesAsFlatArray();
  // map old to new vertex index (-1 means vertex is unused)
  std::vector<int> newVertexIndices(v.size() / 2, -1);
  int newVertexCount{0};
  std::vector<std::size_t> numElements;
  // add triangles from each compartment
  for (const auto &triangles : mesh.getTriangleIndices()) {
    numElements.push_back(triangles.size());
    for (const auto &triangle : triangles) {
      insertTriangle(triangle, factory, newVertexIndices, newVertexCount);
    }
  }
  // add vertices
  std::vector<double> newFlatVertices(
      2 * static_cast<std::size_t>(newVertexCount), 0.0);
  for (std::size_t iOld = 0; iOld < newVertexIndices.size(); ++iOld) {
    auto iNew{newVertexIndices[iOld]};
    if (iNew >= 0) {
      newFlatVertices[2 * static_cast<std::size_t>(iNew)] = v[2 * iOld];
      newFlatVertices[2 * static_cast<std::size_t>(iNew) + 1] = v[2 * iOld + 1];
    }
  }
  for (std::size_t i = 0; i < newFlatVertices.size() / 2; ++i) {
    factory.insertVertex({newFlatVertices[2 * i], newFlatVertices[2 * i + 1]});
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

template <class HostGrid, class MDGTraits>
auto makeDuneGrid(const mesh::Mesh &mesh) {
  auto [numElements, hostGrid] = detail::makeHostGrid<HostGrid>(mesh);
  auto grid =
      detail::makeGrid<HostGrid, MDGTraits>(*hostGrid, numElements.size());
  detail::assignElements(grid, numElements);
  return std::make_pair(grid, hostGrid);
}

} // namespace simulate

} // namespace sme

//  - makeDuneGrid(): create dune-copasi grid from mesh
//  - based on MultiDomainGmshReader from dune-copasi
//  - Note: ensure any changes to MultiDomainGmshReader in future versions of
//  dune-copasi are taken into account here if relevant

#pragma once

#include "mesh.hpp"
#include <array>
#include <cstddef>
#include <dune/geometry/type.hh>
#include <dune/grid/common/gridfactory.hh>
#include <dune/grid/multidomaingrid/multidomaingrid.hh>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

static inline bool
useCompartment(std::size_t compIndex,
               const std::unordered_set<int> &compartmentIndicies) {
  return compartmentIndicies.empty() ||
         compartmentIndicies.find(static_cast<int>(compIndex)) !=
             compartmentIndicies.cend();
}

template <typename HostGrid>
static void insertTriangle(const std::array<std::size_t, 3> &t,
                           Dune::GridFactory<HostGrid> &factory) {
  factory.insertElement(Dune::GeometryTypes::triangle,
                        {static_cast<unsigned int>(t[0]),
                         static_cast<unsigned int>(t[1]),
                         static_cast<unsigned int>(t[2])});
}

template <typename HostGrid>
static void insertRectangle(const std::array<std::size_t, 4> &r,
                            Dune::GridFactory<HostGrid> &factory) {
  // https://www.dune-project.org/doxygen/2.7.0/group__GeometryReferenceElements.html
  factory.insertElement(
      Dune::GeometryTypes::quadrilateral,
      {static_cast<unsigned int>(r[0]), static_cast<unsigned int>(r[1]),
       static_cast<unsigned int>(r[3]), static_cast<unsigned int>(r[2])});
}

template <typename HostGrid>
static std::pair<std::vector<std::size_t>, std::shared_ptr<HostGrid>>
makeHostGrid(const mesh::Mesh &mesh,
             const std::unordered_set<int> &compartmentIndicies) {
  Dune::GridFactory<HostGrid> factory;
  // add vertices
  auto v = mesh.getVerticesAsFlatArray();
  for (std::size_t i = 0; i < v.size() / 2; ++i) {
    factory.insertVertex({v[2 * i], v[2 * i + 1]});
  }
  std::vector<std::size_t> numElements;
  // add compartments containing triangular elements
  std::size_t compIndex{1};
  for (const auto &triangles : mesh.getTriangleIndices()) {
    if (useCompartment(compIndex, compartmentIndicies)) {
      numElements.push_back(triangles.size());
      for (const auto &triangle : triangles) {
        insertTriangle(triangle, factory);
      }
    }
    ++compIndex;
  }
  // add membrane compartments containing rectangular elements
  for (const auto &rectangles : mesh.getRectangleIndices()) {
    if (useCompartment(compIndex, compartmentIndicies)) {
      numElements.push_back(rectangles.size());
      for (const auto &r : rectangles) {
        insertRectangle(r, factory);
      }
    }
    ++compIndex;
  }
  return {numElements, factory.createGrid()};
}

template <typename HostGrid, typename MDGTraits>
static auto makeGrid(HostGrid &hostGrid, std::size_t maxSubdomains) {
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
static void assignElements(Grid &grid,
                           const std::vector<std::size_t> &numElements) {
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

namespace simulate {

template <class HostGrid, class MDGTraits>
auto makeDuneGrid(const mesh::Mesh &mesh,
                  const std::unordered_set<int> &compartmentIndicies) {
  auto [numElements, hostGrid] =
      makeHostGrid<HostGrid>(mesh, compartmentIndicies);
  auto grid = makeGrid<HostGrid, MDGTraits>(*hostGrid, numElements.size());
  assignElements(grid, numElements);
  return std::make_pair(grid, hostGrid);
}

} // namespace simulate

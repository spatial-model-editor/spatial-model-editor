// Dune Implementation base class
//  - defines interface
//  - provides grid

#pragma once

#include "dune_headers.hpp"

namespace simulate {

class DuneConverter;

class DuneImpl {
public:
  static constexpr int DuneDimensions = 2;
  using HostGrid = Dune::UGGrid<DuneDimensions>;
  using MDGTraits =
      Dune::mdgrid::DynamicSubDomainCountTraits<DuneDimensions, 1>;
  using Grid = Dune::mdgrid::MultiDomainGrid<HostGrid, MDGTraits>;
  using GridView = Grid::LeafGridView;
  using SubGrid = Grid::SubDomainGrid;
  using SubGridView = SubGrid::LeafGridView;
  using Elem = decltype(*(elements(std::declval<SubGridView>()).begin()));
  Dune::ParameterTree config;
  std::shared_ptr<Grid> grid;
  explicit DuneImpl(const simulate::DuneConverter &dc);
  virtual ~DuneImpl();
  virtual void setInitial(const simulate::DuneConverter &dc) = 0;
  virtual void run(double time, double maxTimestep) = 0;
  virtual void updateGridFunctions(std::size_t compartmentIndex,
                                   std::size_t nSpecies) = 0;
  virtual double evaluateGridFunction(
      std::size_t iSpecies, const Elem &e,
      const Dune::FieldVector<double, 2> &localPoint) const = 0;

private:
  std::shared_ptr<HostGrid> hostGrid;
};

} // namespace simulate

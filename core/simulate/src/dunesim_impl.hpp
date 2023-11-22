// Dune Implementation

#pragma once

#include "dune_headers.hpp"

namespace sme::simulate {

struct DuneOptions;
class DuneConverter;

class DuneImpl {
public:
  static constexpr int DuneDimensions = 2;
  using HostGrid = Dune::UGGrid<DuneDimensions>;
  using MDGTraits =
      Dune::mdgrid::DynamicSubDomainCountTraits<DuneDimensions, 10>;
  using Grid = Dune::mdgrid::MultiDomainGrid<HostGrid, MDGTraits>;
  using GridView = Grid::LeafGridView;
  using SubGrid = Grid::SubDomainGrid;
  using SubGridView = SubGrid::LeafGridView;
  using Elem = decltype(*(elements(std::declval<SubGridView>()).begin()));
  using Model =
      Dune::Copasi::Model<Grid, Grid::SubDomainGrid::Traits::LeafGridView,
                          double, double>;
  Dune::ParameterTree config;
  std::shared_ptr<Grid> grid;
  std::unordered_map<std::string, std::vector<std::string>> speciesNames;
  std::shared_ptr<Model> model;
  std::shared_ptr<Model::State> state;
  std::unordered_map<std::string, Model::GridFunction> gridFunctions;
  std::unique_ptr<
      Dune::Copasi::SimpleAdaptiveStepper<Model::State, double, double>>
      stepper;
  std::unique_ptr<Dune::PDELab::OneStep<Model::State>> step_operator;
  double t0{0.0};
  double dt{1e-3};
  std::string vtkFilename{};
  explicit DuneImpl(const DuneConverter &dc, const DuneOptions &options);
  ~DuneImpl() = default;
  void setInitial(const DuneConverter &dc);
  void run(double time);
  void updateGridFunctions(const std::string &compartmentName);
  [[nodiscard]] double
  evaluateGridFunction(const std::string &speciesName, const Elem &e,
                       const Dune::FieldVector<double, 2> &localPoint) const;

private:
  std::shared_ptr<HostGrid> hostGrid;
};

} // namespace sme::simulate

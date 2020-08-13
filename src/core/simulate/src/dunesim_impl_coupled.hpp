// Dune Implementation for Coupled Compartments Model
//  - provides model

#pragma once

#include "duneconverter.hpp"
#include "dunefunction.hpp"
#include "dunesim_impl.hpp"
#include <memory>

namespace simulate {

template <int DuneFEMOrder> class DuneImplCoupled : public DuneImpl {
public:
  using ModelTraits =
      Dune::Copasi::ModelMultiDomainP0PkDiffusionReactionTraits<Grid,
                                                                DuneFEMOrder>;
  using Model = Dune::Copasi::ModelMultiDomainDiffusionReaction<ModelTraits>;
  using GF = std::remove_reference_t<decltype(
      *std::declval<Model>().get_grid_function(0, 0).get())>;
  std::unique_ptr<Model> model;
  std::vector<std::shared_ptr<const GF>> gridFunctions;
  explicit DuneImplCoupled(const simulate::DuneConverter &dc)
      : DuneImpl(dc),
        model(std::make_unique<Model>(grid, config.sub("model"))) {
    SPDLOG_INFO("Order: {}", DuneFEMOrder);
  }
  ~DuneImplCoupled() override = default;
  void setInitial(const simulate::DuneConverter &dc) override {
    model->set_initial(makeModelDuneFunctions<GridView>(dc));
  }
  void run(double time, double maxTimestep) override {
    model->suggest_timestep(maxTimestep);
    model->end_time() = model->current_time() + time;
    model->run();
  }
  void updateGridFunctions(std::size_t compartmentIndex,
                           std::size_t nSpecies) override {
    // get grid function for each species in this compartment
    gridFunctions.clear();
    gridFunctions.reserve(nSpecies);
    for (std::size_t iSpecies = 0; iSpecies < nSpecies; ++iSpecies) {
      gridFunctions.push_back(
          model->get_grid_function(compartmentIndex, iSpecies));
    }
  }
  double evaluateGridFunction(
      std::size_t iSpecies, const Elem &e,
      const Dune::FieldVector<double, 2> &localPoint) const override {
    typename GF::Traits::RangeType result;
    gridFunctions[iSpecies]->evaluate(e, localPoint, result);
    return result[0];
  }
};

} // namespace simulate

// Dune Implementation for Independent Compartments Model
//  - provides model

#pragma once

#include "duneconverter.hpp"
#include "dunefunction.hpp"
#include "dunesim_impl.hpp"
#include <memory>

namespace simulate {

template <int DuneFEMOrder> class DuneImplIndependent : public DuneImpl {
public:
  using ModelTraits = Dune::Copasi::ModelPkDiffusionReactionTraits<
      Grid::SubDomainGrid, Grid::SubDomainGrid::Traits::LeafGridView,
      DuneFEMOrder>;
  using Model = Dune::Copasi::ModelDiffusionReaction<ModelTraits>;
  using GF = std::remove_reference_t<decltype(
      *std::declval<Model>().get_grid_function(0).get())>;
  std::vector<std::unique_ptr<Model>> models;
  std::vector<std::shared_ptr<const GF>> gridFunctions;

  explicit DuneImplIndependent(const simulate::DuneConverter &dc)
      : DuneImpl(dc) {
    SPDLOG_INFO("Order: {}", DuneFEMOrder);
    // construct separate model from ini and grid for each compartment
    const auto &modelConfig = config.sub("model", true);
    const auto &dataConfig = modelConfig.sub("data");
    for (const auto &compartmentName :
         modelConfig.sub("compartments").getValueKeys()) {
      int compartmentIndex =
          modelConfig.sub("compartments").template get<int>(compartmentName);
      SPDLOG_INFO("{}[{}]", compartmentName, compartmentIndex);
      auto compartmentConfig = modelConfig.sub(compartmentName, true);
      for (const auto &key : dataConfig.getValueKeys()) {
        compartmentConfig["data." + key] = dataConfig[key];
      }
      auto compartmentGrid =
          Dune::stackobject_to_shared_ptr(grid->subDomain(compartmentIndex));
      models.push_back(
          std::make_unique<Model>(compartmentGrid, compartmentConfig));
    }
  }
  ~DuneImplIndependent() override = default;
  void setInitial(const simulate::DuneConverter &dc) override {
    for (std::size_t i = 0; i < dc.getConcentrations().size(); ++i) {
      models[i]->set_initial(makeCompartmentDuneFunctions<SubGridView>(dc, i));
    }
  }
  void run(double time, double maxTimestep) override {
    for (const auto &model : models) {
      model->suggest_timestep(maxTimestep);
      model->end_time() = model->current_time() + time;
      model->run();
    }
  }
  void updateGridFunctions(std::size_t compartmentIndex,
                           std::size_t nSpecies) override {
    // get grid function for each species in this compartment
    gridFunctions.clear();
    gridFunctions.reserve(nSpecies);
    for (std::size_t iSpecies = 0; iSpecies < nSpecies; ++iSpecies) {
      gridFunctions.push_back(
          models[compartmentIndex]->get_grid_function(iSpecies));
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

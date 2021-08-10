// Dune Implementation for Independent Compartments Model
//  - provides model

#pragma once

#include "duneconverter.hpp"
#include "dunefunction.hpp"
#include "dunesim_impl.hpp"
#include "simulate_options.hpp"
#include <memory>
#include <type_traits>

namespace sme {

namespace simulate {

template <int DuneFEMOrder> class DuneImplIndependent : public DuneImpl {
public:
  using ModelTraits = Dune::Copasi::ModelPkDiffusionReactionTraits<
      Grid::SubDomainGrid, Grid::SubDomainGrid::Traits::LeafGridView,
      DuneFEMOrder>;
  using Model = Dune::Copasi::ModelDiffusionReaction<ModelTraits>;
  using GF = std::remove_reference_t<
      decltype(*std::declval<Model>().get_grid_function(0).get())>;
  std::vector<std::unique_ptr<Model>> models;
  std::vector<std::shared_ptr<const GF>> gridFunctions;
  double t0{0.0};
  std::vector<double> dts;
  std::string vtkFilename{};
  explicit DuneImplIndependent(const DuneConverter &dc,
                               const DuneOptions &options)
      : DuneImpl(dc) {
    SPDLOG_INFO("Order: {}", DuneFEMOrder);
    auto stages =
        Dune::Copasi::BitFlags<Dune::Copasi::ModelSetup::Stages>::all_flags();
    if (options.writeVTKfiles) {
      vtkFilename =
          configs[0].sub("model").template get<std::string>("writer.file_path");
      //    todo: should this be a different filename for each model?
    } else {
      stages.reset(Dune::Copasi::ModelSetup::Stages::Writer);
    }

    for (std::size_t compartmentIndex = 0;
         compartmentIndex < dc.getIniFiles().size(); ++compartmentIndex) {
      SPDLOG_INFO("compartment {}", compartmentIndex);
      auto compartmentGrid = Dune::stackobject_to_shared_ptr(
          grid->subDomain(static_cast<int>(compartmentIndex)));
      models.push_back(std::make_unique<Model>(
          compartmentGrid, configs[compartmentIndex].sub("model"), stages));
      dts.push_back(configs[compartmentIndex]
                        .sub("model.time_stepping")
                        .template get<double>("initial_step"));
    }
  }
  ~DuneImplIndependent() override = default;
  void setInitial(const DuneConverter &dc) override {
    for (std::size_t i = 0; i < dc.getConcentrations().size(); ++i) {
      models[i]->set_initial(makeCompartmentDuneFunctions<SubGridView>(dc, i));
    }
  }
  void run(double time) override {
    auto write_output = [&f = vtkFilename](const auto &state) {
      if (!f.empty()) {
        state.write(f, true);
      }
    };
    auto stepper{Dune::Copasi::make_default_stepper(
        configs[0].sub("model.time_stepping"))};
    for (std::size_t i = 0; i < models.size(); ++i) {
      auto *model = models[i].get();
      double &dt = dts[i];
      stepper.evolve(*model, dt, t0 + time, write_output);
    }
    t0 += time;
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
  [[nodiscard]] double evaluateGridFunction(
      std::size_t iSpecies, const Elem &e,
      const Dune::FieldVector<double, 2> &localPoint) const override {
    typename GF::Traits::RangeType result;
    gridFunctions[iSpecies]->evaluate(e, localPoint, result);
    return result[0];
  }
};

} // namespace simulate

} // namespace sme

// Dune Implementation for Coupled Compartments Model
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

template <int DuneFEMOrder> class DuneImplCoupled : public DuneImpl {
public:
  using ModelTraits =
      Dune::Copasi::ModelMultiDomainPkDiffusionReactionTraits<Grid,
                                                              DuneFEMOrder>;
  using Model = Dune::Copasi::ModelMultiDomainDiffusionReaction<ModelTraits>;
  using GF = std::remove_reference_t<
      decltype(*std::declval<Model>().get_grid_function(0, 0).get())>;
  std::unique_ptr<Model> model;
  std::vector<std::shared_ptr<const GF>> gridFunctions;
  double t0{0.0};
  double dt{1e-3};
  std::string vtkFilename{};
  explicit DuneImplCoupled(const DuneConverter &dc, const DuneOptions &options)
      : DuneImpl(dc) {
    SPDLOG_INFO("Order: {}", DuneFEMOrder);
    auto stages =
        Dune::Copasi::BitFlags<Dune::Copasi::ModelSetup::Stages>::all_flags();
    if (options.writeVTKfiles) {
      vtkFilename =
          configs[0].sub("model").template get<std::string>("writer.file_path");
    } else {
      stages.reset(Dune::Copasi::ModelSetup::Stages::Writer);
    }
    model = std::make_unique<Model>(grid, configs[0].sub("model"), stages);
    dt = configs[0]
             .sub("model.time_stepping")
             .template get<double>("initial_step");
  }
  ~DuneImplCoupled() override = default;
  void setInitial(const DuneConverter &dc) override {
    model->set_initial(makeModelDuneFunctions<GridView>(dc));
  }
  void run(double time) override {
    auto write_output = [&f = vtkFilename](const auto &state) {
      if (!f.empty()) {
        state.write(f, true);
      }
    };
    auto stepper{Dune::Copasi::make_default_stepper(
        configs[0].sub("model.time_stepping"))};
    stepper.evolve(*model.get(), dt, t0 + time, write_output);
    t0 += time;
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

} // namespace sme

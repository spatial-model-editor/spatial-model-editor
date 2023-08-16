#include "dunesim_impl.hpp"
#include "dune_headers.hpp"
#include "dunefunction.hpp"
#include "dunegrid.hpp"
#include "sme/duneconverter.hpp"
#include "sme/logger.hpp"
#include "sme/simulate_options.hpp"
#include <memory>

namespace sme::simulate {

DuneImpl::DuneImpl(const DuneConverter &dc, const DuneOptions &options)
    : speciesNames{dc.getSpeciesNames()} {
  if (SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_DEBUG) {
    // for debug GUI builds enable verbose DUNE logging
    spdlog::set_level(spdlog::level::trace);
  } else {
    // for release GUI builds disable DUNE logging
    spdlog::set_level(spdlog::level::off);
  }
  std::stringstream ssIni(dc.getIniFile().toStdString());
  Dune::ParameterTreeParser::readINITree(ssIni, config);
  std::tie(grid, hostGrid) = makeDuneGrid<HostGrid, MDGTraits>(*dc.getMesh());
  if (options.writeVTKfiles) {
    vtkFilename = "vtk";
  }
  SPDLOG_INFO("parser");
  auto parser_context = std::make_shared<Dune::Copasi::ParserContext>(
      config.sub("parser_context"));
  auto parser_type = Dune::Copasi::string2parser.at(
      config.get("model.parser_type", Dune::Copasi::default_parser_str));
  auto functor_factory =
      std::make_shared<Dune::Copasi::FunctorFactoryParser<DuneDimensions>>(
          parser_type, std::move(parser_context));
  SPDLOG_INFO("model");
  model = Dune::Copasi::make_model<Model>(config.sub("model"), functor_factory);
  dt = options.dt;
  SPDLOG_INFO("state");
  state = model->make_state(grid, config.sub("model"));
  SPDLOG_INFO("stepper");
  stepper = std::make_unique<
      Dune::Copasi::SimpleAdaptiveStepper<Model::State, double, double>>(
      options.decrease, options.increase, options.minDt, options.maxDt);
  SPDLOG_INFO("step_operator");
  step_operator = model->make_step_operator(*state, config.sub("model"));
  SPDLOG_INFO("done");
}

void DuneImpl::setInitial(const DuneConverter &dc) {
  SPDLOG_INFO("Initial condition functions:");
  auto initialConditionFunctions{
      makeModelDuneFunctions<Model::Grid, Model::GridFunction>(dc, *grid)};
  for (const auto &[k, v] : initialConditionFunctions) {
    SPDLOG_INFO("  - {}", k);
  }
  model->interpolate(*state, initialConditionFunctions);
  SPDLOG_INFO("end of interpolate");
}

void DuneImpl::run(double time) {
  stepper->evolve(*step_operator, *state, *state, dt, t0 + time).or_throw();
  if (!vtkFilename.empty()) {
    model->write_vtk(*state, vtkFilename, true);
  }
  t0 += time;
}

void DuneImpl::updateGridFunctions(const std::string &compartmentName) {
  // get grid function for each species in this compartment
  gridFunctions.clear();
  for (const auto &speciesName : speciesNames[compartmentName]) {
    if (!speciesName.empty()) {
      SPDLOG_TRACE("Getting compartment function for species {}", speciesName);
      gridFunctions[speciesName] =
          model->make_compartment_function(state, speciesName);
    }
  }
}

[[nodiscard]] double DuneImpl::evaluateGridFunction(
    const std::string &speciesName, const Elem &e,
    const Dune::FieldVector<double, 2> &localPoint) const {
  return gridFunctions.at(speciesName)(e.geometry().global(localPoint));
}

} // namespace sme::simulate

// Python.h (#included by nanobind.h) must come first
// https://docs.python.org/3.2/c-api/intro.html#include-files
#include <nanobind/nanobind.h>

#include "sme/image_stack.hpp"
#include "sme/logger.hpp"
#include "sme_common.hpp"
#include "sme_model.hpp"
#include <QElapsedTimer>
#include <QFile>
#include <QImage>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

namespace pysme {

void bindModel(nanobind::module_ &m) {
  nanobind::enum_<::sme::simulate::DuneDiscretizationType>(
      m, "DuneDiscretizationType")
      .value("FEM1", ::sme::simulate::DuneDiscretizationType::FEM1);
  nanobind::class_<Model> model(m, "Model",
                                R"(
                                     the spatial model
                                     )");
  nanobind::enum_<::sme::simulate::SimulatorType>(m, "SimulatorType")
      .value("DUNE", ::sme::simulate::SimulatorType::DUNE)
      .value("Pixel", ::sme::simulate::SimulatorType::Pixel);
  nanobind::enum_<::sme::simulate::PixelIntegratorType>(m,
                                                        "PixelIntegratorType")
      .value("RK101", ::sme::simulate::PixelIntegratorType::RK101)
      .value("RK212", ::sme::simulate::PixelIntegratorType::RK212)
      .value("RK323", ::sme::simulate::PixelIntegratorType::RK323)
      .value("RK435", ::sme::simulate::PixelIntegratorType::RK435);
  nanobind::enum_<::sme::simulate::PixelBackendType>(m, "PixelBackendType")
      .value("CPU", ::sme::simulate::PixelBackendType::CPU)
      .value("CUDA", ::sme::simulate::PixelBackendType::CUDA);
  nanobind::class_<::sme::simulate::PixelIntegratorError>(
      m, "PixelIntegratorError")
      .def(nanobind::init<>())
      .def_rw("abs", &::sme::simulate::PixelIntegratorError::abs)
      .def_rw("rel", &::sme::simulate::PixelIntegratorError::rel);
  nanobind::class_<::sme::simulate::DuneOptions>(m, "DuneOptions")
      .def(nanobind::init<>())
      .def_rw("discretization", &::sme::simulate::DuneOptions::discretization)
      .def_rw("integrator", &::sme::simulate::DuneOptions::integrator)
      .def_rw("dt", &::sme::simulate::DuneOptions::dt)
      .def_rw("min_dt", &::sme::simulate::DuneOptions::minDt)
      .def_rw("max_dt", &::sme::simulate::DuneOptions::maxDt)
      .def_rw("increase", &::sme::simulate::DuneOptions::increase)
      .def_rw("decrease", &::sme::simulate::DuneOptions::decrease)
      .def_rw("write_vtk_files", &::sme::simulate::DuneOptions::writeVTKfiles)
      .def_rw("newton_rel_err", &::sme::simulate::DuneOptions::newtonRelErr)
      .def_rw("newton_abs_err", &::sme::simulate::DuneOptions::newtonAbsErr)
      .def_rw("linear_solver", &::sme::simulate::DuneOptions::linearSolver)
      .def_rw("max_threads", &::sme::simulate::DuneOptions::maxThreads);
  nanobind::class_<::sme::simulate::PixelOptions>(m, "PixelOptions")
      .def(nanobind::init<>())
      .def_rw("backend", &::sme::simulate::PixelOptions::backend)
      .def_rw("integrator", &::sme::simulate::PixelOptions::integrator)
      .def_rw("max_err", &::sme::simulate::PixelOptions::maxErr)
      .def_rw("max_timestep", &::sme::simulate::PixelOptions::maxTimestep)
      .def_rw("enable_multithreading",
              &::sme::simulate::PixelOptions::enableMultiThreading)
      .def_rw("max_threads", &::sme::simulate::PixelOptions::maxThreads)
      .def_rw("do_cse", &::sme::simulate::PixelOptions::doCSE)
      .def_rw("opt_level", &::sme::simulate::PixelOptions::optLevel);
  nanobind::class_<::sme::simulate::Options>(m, "SimulationOptions")
      .def(nanobind::init<>())
      .def_rw("dune", &::sme::simulate::Options::dune)
      .def_rw("pixel", &::sme::simulate::Options::pixel);
  nanobind::class_<SimulationSettings>(m, "SimulationSettings",
                                       R"(
      simulation settings used by :func:`Model.simulate`
      )")
      .def(nanobind::init<>())
      .def_rw("times", &SimulationSettings::times,
              R"(
              list[tuple[int, float]]: list of `(n_steps, step_size)` simulation stages
              )")
      .def_rw("options", &SimulationSettings::options,
              R"(
              SimulationOptions: simulator-specific options
              )")
      .def_rw("simulator_type", &SimulationSettings::simulatorType,
              R"(
              SimulatorType: simulator to use
              )");
  model.def(nanobind::init<const std::string &>(), nanobind::arg("filename"))
      .def_prop_rw("name", &Model::getName, &Model::setName,
                   R"(
                    str: the name of this model
                    )")
      .def_prop_rw("simulation_settings",
                   static_cast<SimulationSettings &(Model::*)()>(
                       &Model::getSimulationSettings),
                   &Model::setSimulationSettings,
                   nanobind::rv_policy::reference_internal,
                   R"(
          SimulationSettings: settings used when running simulations

          Modify this object and assign it back to update the model defaults:

          >>> import sme
          >>> model = sme.open_example_model()
          >>> settings = model.simulation_settings
          >>> settings.times = [(2, 0.1)]
          >>> settings.simulator_type = sme.SimulatorType.Pixel
          >>> settings.options.pixel.enable_multithreading = True
          >>> settings.options.pixel.max_threads = 4
          >>> model.simulation_settings = settings
          )")
      .def("export_sbml_file", &Model::exportSbmlFile,
           nanobind::arg("filename"),
           R"(
           exports the model as a spatial SBML file

           Args:
               filename (str): the name of the file to create
           )")
      .def("export_sme_file", &Model::exportSmeFile, nanobind::arg("filename"),
           R"(
           exports the model as a sme file

           Args:
               filename (str): the name of the file to create
           )")
      .def_ro("compartments", &Model::compartments,
              nanobind::rv_policy::reference_internal,
              R"(
                    CompartmentList: the compartments in this model

                    a list of :class:`Compartment` that can be iterated over,
                    or indexed into by name or position in the list.

                    Examples:
                        the list of compartments can be iterated over:

                        >>> import sme
                        >>> model = sme.open_example_model()
                        >>> for compartment in model.compartments:
                        ...     print(compartment.name)
                        Outside
                        Cell
                        Nucleus

                        or a compartment can be found using its name:

                        >>> cell = model.compartments["Cell"]
                        >>> print(cell.name)
                        Cell

                        or indexed by its position in the list:

                        >>> last_compartment = model.compartments[-1]
                        >>> print(last_compartment.name)
                        Nucleus
           )")
      .def_ro("membranes", &Model::membranes,
              R"(
                    MembraneList: the membranes in this model

                    a list of :class:`Membrane` that can be iterated over,
                    or indexed into by name or position in the list.

                    Examples:
                        the list of membranes can be iterated over:

                        >>> import sme
                        >>> model = sme.open_example_model()
                        >>> for membrane in model.membranes:
                        ...     print(membrane.name)
                        Outside <-> Cell
                        Cell <-> Nucleus

                        or a membrane can be found using its name:

                        >>> outer = model.membranes["Outside <-> Cell"]
                        >>> print(outer.name)
                        Outside <-> Cell

                        or indexed by its position in the list:

                        >>> last_membrane = model.membranes[-1]
                        >>> print(last_membrane.name)
                        Cell <-> Nucleus
                    )")
      .def_ro("parameters", &Model::parameters,
              R"(
                    ParameterList: the parameters in this model

                    a list of :class:`Parameter` that can be iterated over,
                    or indexed into by name or position in the list.

                    Examples:
                        the list of parameters can be iterated over:

                        >>> import sme
                        >>> model = sme.open_example_model()
                        >>> for parameter in model.parameters:
                        ...     print(parameter.name)
                        param

                        or a parameter can be found using its name:

                        >>> p = model.parameters["param"]
                        >>> print(p.name)
                        param

                        or indexed by its position in the list:

                        >>> last_param = model.parameters[-1]
                        >>> print(last_param.name)
                        param
                    )")
      .def_prop_ro("compartment_image", &Model::compartment_image,
                   nanobind::rv_policy::take_ownership,
                   R"(
                    numpy.ndarray: an image of the compartments in this model

                    An array of RGB integer values for each voxel in the image of
                    the compartments in this model,
                    which can be displayed using e.g. ``matplotlib.pyplot.imshow``

                    Examples:
                        the image is a 4d (depth x height x width x 3) array of integers:

                        >>> import sme
                        >>> model = sme.open_example_model()
                        >>> type(model.compartment_image)
                        <class 'numpy.ndarray'>
                        >>> model.compartment_image.dtype
                        dtype('uint8')
                        >>> model.compartment_image.shape
                        (1, 100, 100, 3)

                        each voxel in the image has a triplet of RGB integer values
                        in the range 0-255:

                        >>> model.compartment_image[0, 34, 36]
                        array([144,  97, 193], dtype=uint8)

                        the first z-slice of the image can be displayed using matplotlib:

                        >>> import matplotlib.pyplot as plt  # doctest: +SKIP
                        >>> imgplot = plt.imshow(model.compartment_image[0])  # doctest: +SKIP)")
      .def("import_geometry_from_image", &Model::importGeometryFromImage,
           nanobind::arg("filename"),
           R"(
           sets the geometry of each compartment to the corresponding pixels in the supplied geometry image

           Note:
               Currently this function assumes that the compartments maintain the same color
               as they had with the previous geometry image. If the new image does not contain
               pixels of each of these colors, the new model geometry will not be valid.
               The volume of a pixel (in physical units) is also unchanged by this function.

           Args:
               filename (str): the name of the geometry image to import
           )")
      .def(
          "simulate",
          [](Model &self, std::optional<double> simulationTime,
             std::optional<double> imageInterval, int timeoutSeconds,
             bool throwOnTimeout,
             std::optional<::sme::simulate::SimulatorType> simulatorType,
             bool continueExistingSimulation, bool returnResults,
             std::optional<int> nThreads,
             std::optional<SimulationSettings> settings) {
            return self.simulateFloat(simulationTime, imageInterval,
                                      timeoutSeconds, throwOnTimeout,
                                      simulatorType, continueExistingSimulation,
                                      returnResults, nThreads, settings);
          },
          nanobind::arg("simulation_time") = nanobind::none(),
          nanobind::arg("image_interval") = nanobind::none(),
          nanobind::arg("timeout_seconds") = 86400,
          nanobind::arg("throw_on_timeout") = true,
          nanobind::arg("simulator_type") = nanobind::none(),
          nanobind::arg("continue_existing_simulation") = false,
          nanobind::arg("return_results") = true,
          nanobind::arg("n_threads") = nanobind::none(), nanobind::kw_only(),
          nanobind::arg("settings") = nanobind::none(),
          R"(
          Run a simulation and optionally return the results.

          :param simulation_time:
              Length of the simulation in model time units, for example
              `5.5`.
          :type simulation_time: float, optional
          :param image_interval:
              Interval between saved images in model time units, for example
              `1.1`.
              If both `simulation_time` and `image_interval` are omitted,
              simulation stages are taken from
              `model.simulation_settings.times` (or from `settings.times` when
              `settings` is provided).
          :type image_interval: float, optional
          :param timeout_seconds:
              Maximum wall-clock runtime in seconds. Default: `86400` (1 day).
          :type timeout_seconds: int
          :param throw_on_timeout:
              If `True`, raise an exception on timeout. Default: `True`.
          :type throw_on_timeout: bool
          :param simulator_type:
              Per-call simulator override. If omitted, use
              `model.simulation_settings.simulator_type`.
          :type simulator_type: sme.SimulatorType, optional
          :param continue_existing_simulation:
              If `True`, continue from existing results. If `False`, start a
              new simulation and discard existing results. Default: `False`.
          :type continue_existing_simulation: bool
          :param return_results:
              If `True`, return simulation results. If `False`, return an empty
              `SimulationResultList`. Default: `True`.
          :type return_results: bool
          :param n_threads:
              Per-call Pixel thread override. `0` means use all available
              threads.
          :type n_threads: int, optional
          :param settings:
              Per-call simulation settings override. If omitted, use
              `model.simulation_settings`.
          :type settings: sme.SimulationSettings, optional
          :returns: simulation results.
          :rtype: SimulationResultList
          :raises RuntimeError: if the simulation times out or fails.
          )")
      .def(
          "simulate",
          [](Model &self, const std::string &simulationTimes,
             const std::string &imageIntervals, int timeoutSeconds,
             bool throwOnTimeout,
             std::optional<::sme::simulate::SimulatorType> simulatorType,
             bool continueExistingSimulation, bool returnResults,
             std::optional<int> nThreads,
             std::optional<SimulationSettings> settings) {
            return self.simulateString(
                simulationTimes, imageIntervals, timeoutSeconds, throwOnTimeout,
                simulatorType, continueExistingSimulation, returnResults,
                nThreads, settings);
          },
          nanobind::arg("simulation_times"), nanobind::arg("image_intervals"),
          nanobind::arg("timeout_seconds") = 86400,
          nanobind::arg("throw_on_timeout") = true,
          nanobind::arg("simulator_type") = nanobind::none(),
          nanobind::arg("continue_existing_simulation") = false,
          nanobind::arg("return_results") = true,
          nanobind::arg("n_threads") = nanobind::none(), nanobind::kw_only(),
          nanobind::arg("settings") = nanobind::none(),
          R"(
          Run a simulation and optionally return the results.

          :param simulation_times:
              Semicolon-delimited simulation stage lengths in model time units,
              for example `"5"` or `"10;100;20"`.
          :type simulation_times: str
          :param image_intervals:
              Semicolon-delimited image intervals in model time units, for
              example `"1"` or `"2;10;0.5"`.
          :type image_intervals: str
          :param timeout_seconds:
              Maximum wall-clock runtime in seconds. Default: `86400` (1 day).
          :type timeout_seconds: int
          :param throw_on_timeout:
              If `True`, raise an exception on timeout. Default: `True`.
          :type throw_on_timeout: bool
          :param simulator_type:
              Per-call simulator override. If omitted, use
              `model.simulation_settings.simulator_type`.
          :type simulator_type: sme.SimulatorType, optional
          :param continue_existing_simulation:
              If `True`, continue from existing results. If `False`, start a
              new simulation and discard existing results. Default: `False`.
          :type continue_existing_simulation: bool
          :param return_results:
              If `True`, return simulation results. If `False`, return an empty
              `SimulationResultList`. Default: `True`.
          :type return_results: bool
          :param n_threads:
              Per-call Pixel thread override. `0` means use all available
              threads.
          :type n_threads: int, optional
          :param settings:
              Per-call simulation settings override. If omitted, use
              `model.simulation_settings`.
          :type settings: sme.SimulationSettings, optional
          :returns: simulation results.
          :rtype: SimulationResultList
          :raises RuntimeError: if the simulation times out or fails.
          )")
      .def("simulation_results", &Model::getSimulationResults,
           R"(
          returns the simulation results.

          Returns:
              SimulationResultList: the simulation results
          )")
      .def("__repr__",
           [](const Model &a) {
             return fmt::format("<sme.Model named '{}'>", a.getName());
           })
      .def("__str__", &Model::getStr);
}

static std::vector<SimulationResult>
constructSimulationResults(const ::sme::simulate::Simulation *sim,
                           bool getDcdt) {
  std::vector<SimulationResult> results;
  const auto timePoints{sim->getTimePoints()};
  results.reserve(timePoints.size());
  for (std::size_t i = 0; i < timePoints.size(); ++i) {
    auto &result = results.emplace_back();
    result.timePoint = timePoints[i];
    auto img = sim->getConcImage(i, {}, true);
    auto shape = img.volume();
    result.concentration_image = toPyImageRgb(img);
    for (std::size_t ci = 0; ci < sim->getCompartmentIds().size(); ++ci) {
      const auto &names{sim->getPyNames(ci)};
      auto concs{sim->getPyConcs(i, ci)};
      for (std::size_t si = 0; si < names.size(); ++si) {
        result.species_concentration[nanobind::str(names[si].data(),
                                                   names[si].size())] =
            as_ndarray(std::move(concs[si]), shape);
      }
      if (getDcdt && i + 1 == timePoints.size()) {
        if (auto dcdts{sim->getPyDcdts(ci)}; !dcdts.empty()) {
          for (std::size_t si = 0; si < names.size(); ++si) {
            result.species_dcdt[nanobind::str(names[si].data(),
                                              names[si].size())] =
                as_ndarray(std::move(dcdts[si]), shape);
          }
        }
      }
    }
  }
  return results;
}

void Model::init() {
  if (!s->getIsValid()) {
    throw std::invalid_argument("Failed to open model: " +
                                s->getErrorMessage().toStdString());
  }
  compartments.clear();
  compartments.reserve(
      static_cast<std::size_t>(s->getCompartments().getIds().size()));
  for (const auto &compartmentId : s->getCompartments().getIds()) {
    compartments.emplace_back(s.get(), compartmentId.toStdString());
  }
  membranes.clear();
  membranes.reserve(
      static_cast<std::size_t>(s->getMembranes().getIds().size()));
  for (const auto &membraneId : s->getMembranes().getIds()) {
    membranes.emplace_back(s.get(), membraneId.toStdString());
  }
  parameters.clear();
  parameters.reserve(
      static_cast<std::size_t>(s->getParameters().getIds().size()));
  for (const auto &paramId : s->getParameters().getIds()) {
    parameters.emplace_back(s.get(), paramId.toStdString());
  }
}

Model::Model(const std::string &filename) {
#if SPDLOG_ACTIVE_LEVEL > SPDLOG_LEVEL_TRACE
  // disable logging
  spdlog::set_level(spdlog::level::critical);
#endif
  if (!filename.empty()) {
    importFile(filename);
  }
}

void Model::importFile(const std::string &filename) {
  s = std::make_unique<::sme::model::Model>();
  s->importFile(filename);
  init();
}

void Model::importSbmlString(const std::string &xml) {
  s = std::make_unique<::sme::model::Model>();
  s->importSBMLString(xml);
  init();
}

std::string Model::getName() const { return s->getName().toStdString(); }

void Model::setName(const std::string &name) { s->setName(name.c_str()); }

const SimulationSettings &Model::getSimulationSettings() const {
  return s->getSimulationSettings();
}

SimulationSettings &Model::getSimulationSettings() {
  return s->getSimulationSettings();
}

void Model::setSimulationSettings(const SimulationSettings &settings) {
  s->getSimulationSettings() = settings;
}

nanobind::ndarray<nanobind::numpy, std::uint8_t>
Model::compartment_image() const {
  return toPyImageRgb(s->getGeometry().getImages());
}

void Model::importGeometryFromImage(const std::string &filename) {
  try {
    s->getGeometry().importGeometryFromImages(
        ::sme::common::ImageStack(filename.c_str()), true);
  } catch (const std::invalid_argument &e) {
    throw std::invalid_argument("Failed to import geometry from image '" +
                                filename + "': " + e.what());
  }
}

void Model::exportSbmlFile(const std::string &filename) {
  s->exportSBMLFile(filename);
}

void Model::exportSmeFile(const std::string &filename) {
  s->exportSMEFile(filename);
}

std::vector<SimulationResult> Model::simulateString(
    std::optional<std::string> lengths, std::optional<std::string> intervals,
    int timeoutSeconds, bool throwOnTimeout,
    std::optional<::sme::simulate::SimulatorType> simulatorType,
    bool continueExistingSimulation, bool returnResults,
    std::optional<int> nThreads,
    const std::optional<SimulationSettings> &simulationSettingsOverride) {
  QElapsedTimer simulationRuntimeTimer;
  simulationRuntimeTimer.start();
  double timeoutMillisecs{static_cast<double>(timeoutSeconds) * 1000.0};
  auto currentTimes{s->getSimulationSettings().times};
  if (!continueExistingSimulation) {
    s->getSimulationData().clear();
    s->getSimulationSettings().times.clear();
  }
  if ((lengths.has_value() && !intervals.has_value()) ||
      (!lengths.has_value() && intervals.has_value())) {
    throw std::invalid_argument(
        "simulation lengths and intervals must both be set or both be omitted");
  }
  auto &effectiveSettings{s->getSimulationSettings()};
  if (simulationSettingsOverride.has_value()) {
    effectiveSettings = simulationSettingsOverride.value();
  }
  if (simulatorType.has_value()) {
    effectiveSettings.simulatorType = simulatorType.value();
  }
  if (nThreads.has_value() && effectiveSettings.simulatorType ==
                                  ::sme::simulate::SimulatorType::Pixel) {
    auto &pixelOpts{effectiveSettings.options.pixel};
    if (nThreads.value() != 1) {
      pixelOpts.enableMultiThreading = true;
      pixelOpts.maxThreads = static_cast<std::size_t>(nThreads.value());
    } else {
      pixelOpts.enableMultiThreading = false;
      pixelOpts.maxThreads = 1;
    }
  }
  std::vector<std::pair<std::size_t, double>> times;
  if (lengths.has_value() && intervals.has_value()) {
    auto parsedTimes{::sme::simulate::parseSimulationTimes(lengths->c_str(),
                                                           intervals->c_str())};
    if (!parsedTimes.has_value()) {
      throw std::invalid_argument("Invalid simulation lengths or intervals");
    }
    times = parsedTimes.value();
  } else if (simulationSettingsOverride.has_value() &&
             !simulationSettingsOverride->times.empty()) {
    times = simulationSettingsOverride->times;
  } else {
    times = currentTimes;
  }
  if (times.empty()) {
    throw std::invalid_argument(
        "No simulation times specified: set simulation_time/image_interval, "
        "or set simulation_settings.times");
  }
  // ensure any existing DUNE objects are destroyed to avoid later segfaults
  sim.reset();
  sim = std::make_unique<::sme::simulate::Simulation>(*(s.get()));
  if (const auto &e = sim->errorMessage(); !e.empty()) {
    throw std::runtime_error(fmt::format("Error in simulation setup: {}", e));
  }
  sim->doMultipleTimesteps(times, timeoutMillisecs, []() {
    if (PyErr_CheckSignals() != 0) {
      throw nanobind::python_error();
    }
    return false;
  });
  if (const auto &e = sim->errorMessage(); throwOnTimeout && !e.empty()) {
    throw std::runtime_error(fmt::format("Error during simulation: {}", e));
  }
  if (returnResults) {
    return constructSimulationResults(sim.get(), true);
  }
  return {};
}

std::vector<SimulationResult> Model::simulateFloat(
    std::optional<double> simulationTime, std::optional<double> imageInterval,
    int timeoutSeconds, bool throwOnTimeout,
    std::optional<::sme::simulate::SimulatorType> simulatorType,
    bool continueExistingSimulation, bool returnResults,
    std::optional<int> nThreads,
    const std::optional<SimulationSettings> &simulationSettingsOverride) {
  if (simulationTime.has_value() != imageInterval.has_value()) {
    throw std::invalid_argument("simulation_time and image_interval must both "
                                "be set or both be omitted");
  }
  if (simulationTime.has_value()) {
    return simulateString(
        QString::number(simulationTime.value(), 'g', 17).toStdString(),
        QString::number(imageInterval.value(), 'g', 17).toStdString(),
        timeoutSeconds, throwOnTimeout, simulatorType,
        continueExistingSimulation, returnResults, nThreads,
        simulationSettingsOverride);
  }
  return simulateString(std::nullopt, std::nullopt, timeoutSeconds,
                        throwOnTimeout, simulatorType,
                        continueExistingSimulation, returnResults, nThreads,
                        simulationSettingsOverride);
}

std::vector<SimulationResult> Model::getSimulationResults() {
  sim.reset();
  sim = std::make_unique<::sme::simulate::Simulation>(*s);
  if (const auto &e{sim->errorMessage()}; !e.empty()) {
    throw std::runtime_error(fmt::format("Error in simulation setup: {}", e));
  }
  return constructSimulationResults(sim.get(), false);
}

std::string Model::getStr() const {
  std::string str("<sme.Model>\n");
  str.append(fmt::format("  - name: '{}'\n", getName()));
  str.append(fmt::format("  - compartments:{}\n", vecToNames(compartments)));
  str.append(fmt::format("  - membranes:{}", vecToNames(membranes)));
  return str;
}

} // namespace pysme

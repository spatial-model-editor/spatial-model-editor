#include "dunesim.hpp"
#include "dune_headers.hpp"
#include "dunefunction.hpp"
#include "dunegrid.hpp"
#include "dunesim_impl.hpp"
#include "sme/duneconverter.hpp"
#include "sme/utils.hpp"
#include <QElapsedTimer>
#include <QFile>
#include <QImage>
#include <QPainter>
#include <algorithm>
#include <numeric>

namespace sme::simulate {

DuneSim::DuneSim(
    const model::Model &sbmlDoc, const std::vector<std::string> &compartmentIds,
    const std::map<std::string, double, std::less<>> &substitutions) {
  try {
    simulate::DuneConverter dc(sbmlDoc, substitutions, false);
    const auto &options{sbmlDoc.getSimulationSettings().options.dune};
    if (options.discretization != DuneDiscretizationType::FEM1) {
      // for now we only support 1st order FEM
      // in future could add:
      //  - 0th order a.k.a. FVM for independent compartment models
      //  - 2nd order FEM for both types of models
      SPDLOG_ERROR("Invalid integrator type requested");
      throw std::runtime_error("Invalid integrator type requested");
    }
    if (dc.getIniFile().isEmpty()) {
      currentErrorMessage = "Nothing to simulate";
      SPDLOG_WARN("{}", currentErrorMessage);
      return;
    }
    if (dc.getMesh() != nullptr) {
      pDuneImpl2d =
          std::make_unique<DuneImpl<2>>(dc, options, sbmlDoc, compartmentIds);
    } else {
      pDuneImpl3d =
          std::make_unique<DuneImpl<3>>(dc, options, sbmlDoc, compartmentIds);
    }
  } catch (const Dune::Exception &e) {
    currentErrorMessage = e.what();
    SPDLOG_ERROR("{}", currentErrorMessage);
  } catch (const std::runtime_error &e) {
    SPDLOG_ERROR("runtime_error: {}", e.what());
    currentErrorMessage = e.what();
  }
}

DuneSim::~DuneSim() = default;

std::size_t DuneSim::run(double time, double timeout_ms,
                         const std::function<bool()> &stopRunningCallback) {
  if (pDuneImpl2d == nullptr && pDuneImpl3d == nullptr) {
    return 0;
  }
  QElapsedTimer timer;
  timer.start();
  try {
    if (pDuneImpl2d != nullptr) {
      pDuneImpl2d->run(time);
    } else {
      pDuneImpl3d->run(time);
    }
    currentErrorMessage.clear();
  } catch (const Dune::Exception &e) {
    currentErrorMessage = e.what();
    SPDLOG_ERROR("{}", currentErrorMessage);
    return 0;
  }
  if (stopRunningCallback && stopRunningCallback()) {
    SPDLOG_DEBUG("Simulation cancelled: requesting stop");
    currentErrorMessage = "Simulation cancelled";
  }
  if (timeout_ms >= 0.0 && static_cast<double>(timer.elapsed()) >= timeout_ms) {
    SPDLOG_DEBUG("Simulation timeout: requesting stop");
    currentErrorMessage = "Simulation timeout";
  }
  return 1;
}

const std::vector<double> &
DuneSim::getConcentrations(std::size_t compartmentIndex) const {
  if (pDuneImpl2d != nullptr) {
    return pDuneImpl2d->getConcentrations(compartmentIndex);
  } else {
    return pDuneImpl3d->getConcentrations(compartmentIndex);
  }
}

std::size_t DuneSim::getConcentrationPadding() const { return 0; }

const std::string &DuneSim::errorMessage() const { return currentErrorMessage; }

const common::ImageStack &DuneSim::errorImages() const {
  return currentErrorImages;
}

void DuneSim::setStopRequested([[maybe_unused]] bool stop) {
  // TODO: implement this properly: duneimpl in separate thread --> send
  // kill/stop signal ?
  SPDLOG_DEBUG("Not implemented - ignoring request");
}

bool DuneSim::getStopRequested() const {
  // TODO: implement this properly: duneimpl in separate thread --> send
  // kill/stop signal ?
  SPDLOG_DEBUG("Not implemented - ignoring request");
  return false;
}

} // namespace sme::simulate

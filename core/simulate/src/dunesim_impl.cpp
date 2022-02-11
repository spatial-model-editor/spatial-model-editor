#include "dunesim_impl.hpp"
#include "dunegrid.hpp"
#include "sme/duneconverter.hpp"
#include "sme/logger.hpp"

namespace sme::simulate {

DuneImpl::DuneImpl(const simulate::DuneConverter &dc) {
  for (const auto &ini : dc.getIniFiles()) {
    std::stringstream ssIni(ini.toStdString());
    auto &config = configs.emplace_back();
    Dune::ParameterTreeParser::readINITree(ssIni, config);
  }
  // init Dune logging if not already done
  if (!Dune::Logging::Logging::initialized()) {
    Dune::Logging::Logging::init(
        Dune::FakeMPIHelper::getCollectiveCommunication(),
        configs[0].sub("logging"));
    if (SPDLOG_ACTIVE_LEVEL > SPDLOG_LEVEL_DEBUG) {
      // for release builds disable DUNE logging
      Dune::Logging::Logging::mute();
    }
  }
  // construct grid
  std::tie(grid, hostGrid) = makeDuneGrid<HostGrid, MDGTraits>(*dc.getMesh());
}

DuneImpl::~DuneImpl() = default;

} // namespace sme::simulate

#include "dunesim_impl.hpp"
#include "duneconverter.hpp"
#include "dunegrid.hpp"
#include "logger.hpp"

namespace simulate {

DuneImpl::DuneImpl(const simulate::DuneConverter &dc) {
  std::stringstream ssIni(dc.getIniFile().toStdString());
  Dune::ParameterTreeParser::readINITree(ssIni, config);

  // init Dune logging if not already done
  if (!Dune::Logging::Logging::initialized()) {
    Dune::Logging::Logging::init(
        Dune::FakeMPIHelper::getCollectiveCommunication(),
        config.sub("logging"));
    if (SPDLOG_ACTIVE_LEVEL >= 2) {
      // for release builds disable DUNE logging
      Dune::Logging::Logging::mute();
    }
  }
  // construct grid
  std::tie(grid, hostGrid) =
      makeDuneGrid<HostGrid, MDGTraits>(*dc.getMesh(), dc.getGMSHCompIndices());
}

DuneImpl::~DuneImpl() = default;

} // namespace simulate

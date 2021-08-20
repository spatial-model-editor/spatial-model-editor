#include "catch_wrapper.hpp"
#include "dune_headers.hpp"
#include "duneconverter.hpp"
#include "dunegrid.hpp"
#include "model.hpp"
#include "model_test_utils.hpp"
#include <QFile>
#include <dune/copasi/grid/multidomain_gmsh_reader.hh>
#include <locale>

using namespace sme;
using namespace sme::test;

constexpr int DuneDimensions{2};
using HostGrid = Dune::UGGrid<DuneDimensions>;
using MDGTraits = Dune::mdgrid::DynamicSubDomainCountTraits<DuneDimensions, 1>;
using Grid = Dune::mdgrid::MultiDomainGrid<HostGrid, MDGTraits>;

static std::vector<double> getAllElementCoordinates(Grid *grid, int subdomain) {
  std::vector<double> v;
  auto gridView = grid->subDomain(subdomain).leafGridView();
  for (const auto &e : elements(gridView)) {
    for (int i = 0; i < e.geometry().corners(); ++i) {
      v.push_back(e.geometry().corner(i)[0]);
      v.push_back(e.geometry().corner(i)[1]);
    }
  }
  return v;
}

SCENARIO("DUNE: grid",
         "[core/simulate/dunegrid][core/simulate][core][dunegrid]") {
  for (auto exampleModel : {Mod::ABtoC, Mod::VerySimpleModel, Mod::LiverSimplified}) {
    CAPTURE(exampleModel);
    // load mesh from model
    auto m{getExampleModel(exampleModel)};
    simulate::DuneConverter dc(m, {}, false);
    const auto *mesh{m.getGeometry().getMesh()};

    // generate dune grid with sim::makeDuneGrid
    auto [grid, hostGrid] = simulate::makeDuneGrid<HostGrid, MDGTraits>(*mesh);

    // generate dune grid with Dune::Copasi::MultiDomainGmshReader
    if (QFile f("grid.msh"); f.open(QIODevice::WriteOnly | QIODevice::Text)) {
      f.write(mesh->getGMSH().toUtf8());
      f.close();
    }
    // note: requires C locale
    std::locale userLocale = std::locale::global(std::locale::classic());
    // init & mute Dune logging
    if (!Dune::Logging::Logging::initialized()) {
      Dune::Logging::Logging::init(
          Dune::FakeMPIHelper::getCollectiveCommunication());
    }
    Dune::Logging::Logging::mute();
    auto [gmshGrid, gmshHostGrid] =
        Dune::Copasi::MultiDomainGmshReader<Grid>::read("grid.msh");
    std::locale::global(userLocale);

    // compare grids
    int nCompartments{grid->maxSubDomainIndex()};
    REQUIRE(gmshGrid->maxSubDomainIndex() == nCompartments);
    for (int compIndex = 0; compIndex < nCompartments; ++compIndex) {
      auto meshCoords = getAllElementCoordinates(grid.get(), compIndex);
      auto gmshCoords = getAllElementCoordinates(gmshGrid.get(), compIndex);
      CAPTURE(compIndex);
      REQUIRE(meshCoords.size() == gmshCoords.size());
      for (std::size_t elemIndex = 0; elemIndex < meshCoords.size();
           ++elemIndex) {
        CAPTURE(elemIndex);
        REQUIRE(meshCoords[elemIndex] == dbl_approx(gmshCoords[elemIndex]));
      }
    }
  }
}

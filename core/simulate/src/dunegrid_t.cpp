#include "catch_wrapper.hpp"
#include "dune_headers.hpp"
#include "dunegrid.hpp"
#include "model_test_utils.hpp"
#include "sme/duneconverter.hpp"
#include "sme/model.hpp"
#include <QFile>
#include <locale>

using namespace sme;
using namespace sme::test;

static Dune::ParameterTree getConfig(const simulate::DuneConverter &dc) {
  Dune::ParameterTree config;
  std::stringstream ssIni(dc.getIniFile().toStdString());
  Dune::ParameterTreeParser::readINITree(ssIni, config);
  return config;
}

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

TEST_CASE("DUNE: grid",
          "[core/simulate/dunegrid][core/simulate][core][dunegrid]") {
  for (auto exampleModel :
       {Mod::ABtoC, Mod::VerySimpleModel, Mod::LiverSimplified}) {
    CAPTURE(exampleModel);
    // load mesh from model
    auto m{getExampleModel(exampleModel)};
    auto filename =
        QString("tmp_dunegrid_model_%1").arg(static_cast<int>(exampleModel));
    simulate::DuneConverter dc(m, {}, true, filename + ".ini");
    auto config{getConfig(dc)};
    const auto *mesh{m.getGeometry().getMesh()};

    // generate dune grid with sim::makeDuneGrid
    auto [grid, hostGrid] = simulate::makeDuneGrid<HostGrid, MDGTraits>(*mesh);

    // generate dune grid with Dune::Copasi::make_multi_domain_grid
    if (QFile f(filename + ".msh");
        f.open(QIODevice::WriteOnly | QIODevice::Text)) {
      f.write(mesh->getGMSH().toUtf8());
      f.close();
    }
    // note: requires C locale
    std::locale userLocale = std::locale::global(std::locale::classic());
    auto gmshGrid{Dune::Copasi::make_multi_domain_grid<Grid>(config)};
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

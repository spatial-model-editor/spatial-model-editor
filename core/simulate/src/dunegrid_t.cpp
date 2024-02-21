#include "catch_wrapper.hpp"
#include "dune_headers.hpp"
#include "dunegrid.hpp"
#include "model_test_utils.hpp"
#include "sme/duneconverter.hpp"
#include "sme/mesh3d.hpp"
#include "sme/model.hpp"
#include "sme/utils.hpp"
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

using HostGrid2d = Dune::UGGrid<2>;
using MDGTraits2d = Dune::mdgrid::FewSubDomainsTraits<2, 64>;
using Grid2d = Dune::mdgrid::MultiDomainGrid<HostGrid2d, MDGTraits2d>;

using HostGrid3d = Dune::UGGrid<3>;
using MDGTraits3d = Dune::mdgrid::FewSubDomainsTraits<3, 64>;
using Grid3d = Dune::mdgrid::MultiDomainGrid<HostGrid3d, MDGTraits3d>;

template <typename Grid>
static std::vector<double> getAllElementCoordinates(Grid *grid, int subdomain) {
  std::vector<double> v;
  auto gridView = grid->subDomain(subdomain).leafGridView();
  for (const auto &e : elements(gridView)) {
    for (int i = 0; i < e.geometry().corners(); ++i) {
      for (double c : e.geometry().corner(i)) {
        v.push_back(c);
      }
    }
  }
  return v;
}

TEST_CASE("DUNE grid",
          "[core/simulate/dunegrid][core/simulate][core][dunegrid][dune]") {
  SECTION("2d") {
    for (auto exampleModel :
         {Mod::ABtoC, Mod::VerySimpleModel, Mod::LiverSimplified}) {
      CAPTURE(exampleModel);
      // load mesh from model
      auto m{getExampleModel(exampleModel)};
      auto filename = QString("tmp_dunegrid2d_model_%1")
                          .arg(static_cast<int>(exampleModel));
      simulate::DuneConverter dc(m, {}, true, filename + ".ini");
      auto config{getConfig(dc)};
      const auto *mesh{m.getGeometry().getMesh()};

      // generate dune grid with our makeDuneGrid function
      auto [grid, hostGrid] =
          simulate::makeDuneGrid<HostGrid2d, MDGTraits2d>(*mesh);

      // generate dune grid with Dune::Copasi::make_multi_domain_grid
      // note: requires C locale
      std::locale userLocale = std::locale::global(std::locale::classic());
      auto gmshGrid{Dune::Copasi::make_multi_domain_grid<Grid2d>(config)};
      std::locale::global(userLocale);

      // compare grids
      auto nCompartments{grid->maxSubDomainIndex()};
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
  SECTION("3d") {
    for (auto exampleModel :
         {Mod::SingleCompartmentDiffusion3D, Mod::VerySimpleModel3D}) {
      CAPTURE(exampleModel);
      // load mesh from model
      auto m{getExampleModel(exampleModel)};
      const auto &geometry = m.getGeometry();
      auto filename = QString("tmp_dunegrid3d_model_%1")
                          .arg(static_cast<int>(exampleModel));
      simulate::DuneConverter dc(m, {}, true, filename + ".ini");
      const auto *mesh3d{m.getGeometry().getMesh3d()};
      REQUIRE(mesh3d != nullptr);
      REQUIRE(mesh3d->isValid());
      auto config{getConfig(dc)};

      // generate dune grid with our makeDuneGrid function
      auto [grid, hostGrid] =
          simulate::makeDuneGrid<HostGrid3d, MDGTraits3d>(*mesh3d);

      // generate dune grid with Dune::Copasi::make_multi_domain_grid
      // note: requires C locale
      std::locale userLocale = std::locale::global(std::locale::classic());
      auto gmshGrid{Dune::Copasi::make_multi_domain_grid<Grid3d>(config)};
      std::locale::global(userLocale);

      // compare grids
      auto nCompartments{grid->maxSubDomainIndex()};
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
}

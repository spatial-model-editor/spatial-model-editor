#include "catch_wrapper.hpp"
#include "dune_headers.hpp"
#include "dunegrid.hpp"
#include "dunesim_impl.hpp"
#include "model_test_utils.hpp"
#include "sme/duneconverter.hpp"
#include "sme/mesh3d.hpp"
#include "sme/model.hpp"
#include "sme/utils.hpp"
#include <QFile>
#include <algorithm>
#include <dune/copasi/grid/cell_data.hh>
#include <dune/grid/common/mcmgmapper.hh>
#include <dune/grid/common/rangegenerators.hh>
#include <fstream>
#include <locale>
#include <numeric>

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
static std::vector<double> getAllElementCoordinates(Grid *grid,
                                                    unsigned int subdomain) {
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
      const auto *mesh{m.getGeometry().getMesh2d()};

      // generate dune grid with our makeDuneGrid function
      auto [grid, hostGrid] =
          simulate::makeDuneGrid<HostGrid2d, MDGTraits2d>(*mesh);

      // generate dune grid with Dune::Copasi::make_multi_domain_grid
      // note: requires C locale
      std::locale userLocale = std::locale::global(std::locale::classic());
      auto gmshGrid{Dune::Copasi::make_multi_domain_grid<Grid2d>(config)};
      std::locale::global(userLocale);

      // compare grids
      auto nCompartments = grid->maxSubDomainIndex();
      REQUIRE(gmshGrid->maxSubDomainIndex() == nCompartments);
      for (unsigned int compIndex = 0; compIndex < nCompartments; ++compIndex) {
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
  SECTION("cell data size matches mesh2d cells") {
    auto m{getExampleModel(Mod::VerySimpleModel)};
    const auto nVoxels = m.getGeometry().getImages().volume().nVoxels();

    std::size_t spatialSpeciesCount = 0;
    std::size_t speciesIndex = 0;
    for (const auto &compartmentId : m.getCompartments().getIds()) {
      for (const auto &speciesId : m.getSpecies().getIds(compartmentId)) {
        if (!m.getSpecies().getIsSpatial(speciesId)) {
          continue;
        }
        ++spatialSpeciesCount;
        std::vector<double> diff(nVoxels, 0.0);
        for (std::size_t i = 0; i < nVoxels; ++i) {
          diff[i] = static_cast<double>((i % 11) + 1 + speciesIndex);
        }
        m.getSpecies().setSampledFieldDiffusionConstant(speciesId, diff);
        ++speciesIndex;
      }
    }

    simulate::DuneConverter dc(m);
    const auto &arrays = dc.getDiffusionConstantArrays();
    REQUIRE(arrays.size() == spatialSpeciesCount);
    const auto *mesh = m.getGeometry().getMesh2d();
    REQUIRE(mesh != nullptr);
    std::size_t meshElementCount = 0;
    for (const auto &comp : mesh->getTriangleIndices()) {
      meshElementCount += comp.size();
    }

    auto [grid, hostGrid] =
        simulate::makeDuneGrid<HostGrid2d, MDGTraits2d>(*mesh);
    auto gridView = grid->leafGridView();
    REQUIRE(gridView.size(0) == meshElementCount);

    auto cell_data = simulate::makeDiffusionCellDataForGridView(dc, gridView);
    REQUIRE(cell_data != nullptr);
    REQUIRE(cell_data->gridview_size() == meshElementCount);
    REQUIRE(cell_data->size() == arrays.size());
  }
  SECTION("cell data size matches mesh3d cells") {
    auto m{getExampleModel(Mod::VerySimpleModel3D)};
    const auto nVoxels = m.getGeometry().getImages().volume().nVoxels();

    std::size_t spatialSpeciesCount = 0;
    std::size_t speciesIndex = 0;
    for (const auto &compartmentId : m.getCompartments().getIds()) {
      for (const auto &speciesId : m.getSpecies().getIds(compartmentId)) {
        if (!m.getSpecies().getIsSpatial(speciesId)) {
          continue;
        }
        ++spatialSpeciesCount;
        std::vector<double> diff(nVoxels, 0.0);
        for (std::size_t i = 0; i < nVoxels; ++i) {
          diff[i] = static_cast<double>((i % 11) + 1 + speciesIndex);
        }
        m.getSpecies().setSampledFieldDiffusionConstant(speciesId, diff);
        ++speciesIndex;
      }
    }

    simulate::DuneConverter dc(m);
    const auto &arrays = dc.getDiffusionConstantArrays();
    REQUIRE(arrays.size() == spatialSpeciesCount);
    const auto *mesh = m.getGeometry().getMesh3d();
    REQUIRE(mesh != nullptr);
    std::size_t meshElementCount = 0;
    for (const auto &comp : mesh->getTetrahedronIndices()) {
      meshElementCount += comp.size();
    }

    auto [grid, hostGrid] =
        simulate::makeDuneGrid<HostGrid3d, MDGTraits3d>(*mesh);
    auto gridView = grid->leafGridView();
    REQUIRE(gridView.size(0) == meshElementCount);

    auto cell_data = simulate::makeDiffusionCellDataForGridView(dc, gridView);
    REQUIRE(cell_data != nullptr);
    REQUIRE(cell_data->gridview_size() == meshElementCount);
    REQUIRE(cell_data->size() == arrays.size());
  }
  SECTION("3d") {
    for (auto exampleModel :
         {Mod::SingleCompartmentDiffusion3D, Mod::VerySimpleModel3D}) {
      CAPTURE(exampleModel);
      // load mesh from model
      auto m{getExampleModel(exampleModel)};
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
      for (unsigned int compIndex = 0; compIndex < nCompartments; ++compIndex) {
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
  SECTION("external cell data files match internal cell data") {
    auto m{getExampleModel(Mod::VerySimpleModel)};
    const auto nVoxels = m.getGeometry().getImages().volume().nVoxels();
    std::size_t speciesIndex = 0;
    for (const auto &compartmentId : m.getCompartments().getIds()) {
      for (const auto &speciesId : m.getSpecies().getIds(compartmentId)) {
        if (!m.getSpecies().getIsSpatial(speciesId)) {
          continue;
        }
        std::vector<double> diff(nVoxels, 0.0);
        for (std::size_t i = 0; i < nVoxels; ++i) {
          diff[i] = static_cast<double>((i % 11) + 1 + speciesIndex);
        }
        m.getSpecies().setSampledFieldDiffusionConstant(speciesId, diff);
        ++speciesIndex;
      }
    }

    const QString iniFilename{"tmp_dunegrid_external.ini"};
    simulate::DuneConverter dc(m, {}, true, iniFilename);
    const auto &arrays = dc.getDiffusionConstantArrays();
    REQUIRE(!arrays.empty());

    const auto *mesh = m.getGeometry().getMesh2d();
    REQUIRE(mesh != nullptr);
    auto [grid, hostGrid] =
        simulate::makeDuneGrid<HostGrid2d, MDGTraits2d>(*mesh);
    auto gridView = grid->leafGridView();
    auto cell_data = simulate::makeDiffusionCellDataForGridView(dc, gridView);
    REQUIRE(cell_data != nullptr);

    Dune::MultipleCodimMultipleGeomTypeMapper<decltype(gridView)> mapper(
        gridView, Dune::mcmgElementLayout());

    auto baseIniFile = iniFilename;
    if (baseIniFile.endsWith(".ini")) {
      baseIniFile.chop(4);
    }
    for (const auto &[name, data] : arrays) {
      (void)data;
      const QString fileName =
          QString("%1_%2.txt").arg(baseIniFile, name.c_str());
      std::ifstream in(fileName.toStdString());
      REQUIRE(in.is_open());
      std::size_t nEntries = 0;
      in >> nEntries;
      REQUIRE(nEntries == mapper.size());
      std::vector<double> values(nEntries, 0.0);
      for (std::size_t i = 0; i < nEntries; ++i) {
        std::size_t idx = 0;
        double v = 0.0;
        in >> idx >> v;
        REQUIRE(idx < values.size());
        values[idx] = v;
      }
      std::size_t keyIndex = 0;
      const auto &keys = cell_data->keys();
      REQUIRE(!keys.empty());
      for (std::size_t i = 0; i < keys.size(); ++i) {
        if (keys[i] == name) {
          keyIndex = i;
          break;
        }
      }
      for (const auto &cell : elements(gridView)) {
        auto idx = mapper.index(cell);
        auto val = cell_data->getData(keyIndex, cell);
        REQUIRE(val.has_value());
        REQUIRE(values[idx] == dbl_approx(*val));
      }
    }
  }
}

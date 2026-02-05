#include "catch_wrapper.hpp"
#include "dune_cell_data.hpp"
#include "dunegrid.hpp"
#include "model_test_utils.hpp"
#include "sme/duneconverter.hpp"
#include "sme/model.hpp"

using namespace sme;
using namespace sme::test;

using HostGrid2d = Dune::UGGrid<2>;
using MDGTraits2d = Dune::mdgrid::FewSubDomainsTraits<2, 64>;
using HostGrid3d = Dune::UGGrid<3>;
using MDGTraits3d = Dune::mdgrid::FewSubDomainsTraits<3, 64>;

TEST_CASE("DUNE cell data helper",
          "[core/simulate/dune_cell_data][core/simulate][core][dune]") {
  auto runTest = [](model::Model &m, auto &gridView) {
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

    simulate::DuneConverter dc(m);
    const auto &arrays = dc.getDiffusionConstantArrays();
    REQUIRE(!arrays.empty());

    auto valuesByName =
        simulate::makeDiffusionCellValuesForGridView(dc, gridView);
    REQUIRE(valuesByName.size() == arrays.size());
    for (const auto &[name, values] : valuesByName) {
      REQUIRE(arrays.contains(name));
      REQUIRE(values.size() == gridView.size(0));
    }

    auto cell_data = simulate::makeDiffusionCellDataForGridView(dc, gridView);
    REQUIRE(cell_data != nullptr);
    REQUIRE(cell_data->size() == valuesByName.size());
    for (const auto &[name, values] : valuesByName) {
      std::size_t keyIndex = 0;
      const auto &keys = cell_data->keys();
      for (std::size_t i = 0; i < keys.size(); ++i) {
        if (keys[i] == name) {
          keyIndex = i;
          break;
        }
      }
      for (const auto &cell : elements(gridView)) {
        auto idx = gridView.indexSet().index(cell);
        auto val = cell_data->getData(keyIndex, cell);
        REQUIRE(val.has_value());
        REQUIRE(values[idx] == dbl_approx(*val));
      }
    }
  };

  SECTION("2d") {
    auto m{getExampleModel(Mod::VerySimpleModel)};
    const auto *mesh = m.getGeometry().getMesh2d();
    REQUIRE(mesh != nullptr);
    auto [grid, hostGrid] =
        simulate::makeDuneGrid<HostGrid2d, MDGTraits2d>(*mesh);
    auto gridView = grid->leafGridView();
    runTest(m, gridView);
  }

  SECTION("3d") {
    auto m{getExampleModel(Mod::VerySimpleModel3D)};
    const auto *mesh = m.getGeometry().getMesh3d();
    REQUIRE(mesh != nullptr);
    auto [grid, hostGrid] =
        simulate::makeDuneGrid<HostGrid3d, MDGTraits3d>(*mesh);
    auto gridView = grid->leafGridView();
    runTest(m, gridView);
  }
}

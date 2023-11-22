// todo: re-enable tiff tests once
// https://gitlab.dune-project.org/copasi/dune-copasi/-/issues/80 is resolved

#include "catch_wrapper.hpp"
#include "dune_headers.hpp"
#include "dunefunction.hpp"
#include "dunegrid.hpp"
#include "model_test_utils.hpp"
#include "sme/duneconverter.hpp"
#include "sme/model.hpp"
#include <QFile>
#include <cmath>
#include <locale>

using namespace sme;
using namespace sme::test;

static constexpr int DuneDimensions = 2;
using HostGrid = Dune::UGGrid<DuneDimensions>;
using MDGTraits = Dune::mdgrid::DynamicSubDomainCountTraits<DuneDimensions, 10>;
using Grid = Dune::mdgrid::MultiDomainGrid<HostGrid, MDGTraits>;
using GridView = Grid::LeafGridView;
using SubGrid = Grid::SubDomainGrid;
using SubGridView = SubGrid::LeafGridView;
using Model =
    Dune::Copasi::Model<Grid, Grid::SubDomainGrid::Traits::LeafGridView, double,
                        double>;

static Dune::ParameterTree getConfig(const simulate::DuneConverter &dc) {
  Dune::ParameterTree config;
  std::stringstream ssIni(dc.getIniFile().toStdString());
  Dune::ParameterTreeParser::readINITree(ssIni, config);
  return config;
}

static double initialAnalyticConcentration(double x, double y) {
  return (1 + x) * (std::cos(x / 141.7) + 1.1) +
         (y + 1) * (std::sin(y / 111.1) + 1.2);
}

struct AvgDiff {
  double AnalyticTiff;
  double AnalyticFunc;
  double TiffFunc;
};

static std::vector<AvgDiff> getAvgDiffs(Mod exampleModel,
                                        std::size_t maxTriangleArea = 5) {
  std::vector<AvgDiff> avgDiffs;
  auto m{getExampleModel(exampleModel)};
  // set initial concentration from analytic expression
  for (const auto &compId : m.getCompartments().getIds()) {
    for (const auto &id : m.getSpecies().getIds(compId)) {
      if (!m.getSpecies().getIsConstant(id)) {
        m.getSpecies().setAnalyticConcentration(
            id, "(1+x)*(cos(x/141.7)+1.1)+(y+1)*(sin(y/111.1)+1.2)");
      }
    }
  }
  auto nCompartments{m.getCompartments().getIds().size()};
  auto &mesh{*(m.getGeometry().getMesh())};
  // make mesh finer to reduce interpolation errors
  for (std::size_t i = 0; i < static_cast<std::size_t>(nCompartments); ++i) {
    mesh.setCompartmentMaxTriangleArea(i, maxTriangleArea);
  }

  const auto &lengthUnit = m.getUnits().getLength();
  const auto &volumeUnit = m.getUnits().getVolume();
  double volOverL3{model::getVolOverL3(lengthUnit, volumeUnit)};

  // create model using GridFunction for initial conditions
  simulate::DuneConverter dc(m, {}, false);
  auto [grid, hostGrid] = simulate::makeDuneGrid<HostGrid, MDGTraits>(mesh);
  auto config{getConfig(dc)};
  // make a parser
  auto parser_context{std::make_shared<Dune::Copasi::ParserContext>(
      config.sub("parser_context"))};
  auto parser_type{Dune::Copasi::string2parser.at(
      config.get("model.parser_type", Dune::Copasi::default_parser_str))};
  auto functor_factory{std::make_shared<Dune::Copasi::FunctorFactoryParser<2>>(
      parser_type, parser_context)};
  auto model{
      Dune::Copasi::make_model<Model>(config.sub("model"), functor_factory)};
  auto initial_state{model->make_state(grid, config.sub("model"))};
  model->interpolate(
      *initial_state,
      simulate::makeModelDuneFunctions<Model::Grid, Model::GridFunction>(
          dc, *grid));

  //  // create model using TIFF files for initial conditions
  //  simulate::DuneConverter dcTiff(m, {}, true);
  //  auto configTiff = getConfig(dcTiff);
  //  auto parser_context_tiff{std::make_shared<Dune::Copasi::ParserContext>(
  //      configTiff.sub("parser_context"))};
  //  auto gridTiff = make_multi_domain_grid<Grid>(configTiff,
  //  parser_context_tiff);
  //  // this is what the dune-copasi binary does after making the grid:
  //  configTiff.sub("model.compartments") = configTiff.sub("compartments");
  //  // make a parser
  //  auto parser_type_tiff{Dune::Copasi::string2parser.at(
  //      configTiff.get("model.parser_type",
  //      Dune::Copasi::default_parser_str))};
  //  auto
  //  functor_factory_tiff{std::make_shared<Dune::Copasi::FunctorFactoryParser<DuneDimensions>>(
  //      parser_type_tiff, parser_context_tiff)};
  //  auto modelTiff{
  //      Dune::Copasi::make_model<Model>(configTiff.sub("model"),
  //      functor_factory_tiff)};
  //  auto initial_state_tiff{modelTiff->make_state(std::move(gridTiff),
  //  configTiff.sub("model"))}; model->interpolate(
  //      *initial_state_tiff,
  //      model->make_initial(*(initial_state_tiff->grid),
  //      configTiff.sub("model")));

  // compare initial species concentrations
  for (int domain = 0; domain < nCompartments; ++domain) {
    for (const auto &species : dc.getSpeciesNames().at(
             m.getCompartments().getIds()[domain].toStdString())) {
      if (!species.empty()) {
        const auto &c{
            m.getSpecies().getSampledFieldConcentration(species.c_str())};
        double tiffULP{*std::max_element(c.cbegin(), c.cend()) / 65536.0 /
                       volOverL3};
        auto gfFunc = model->make_compartment_function(*initial_state, species);
        //      auto gfTiff =
        //          modelTiff->make_compartment_function(*initial_state_tiff,
        //          species);
        double avgDiffAnalyticTiff{0.0};
        double avgDiffAnalyticFunc{0.0};
        double avgDiffTiffFunc{0.0};
        double n{0.0};
        for (const auto &e : elements(
                 grid->subDomain(static_cast<int>(domain)).leafGridView())) {
          for (double x : {0.0}) {
            for (double y : {0.0}) {
              Dune::FieldVector<double, 2> local{x, y};
              auto globalPos = e.geometry().global(local);
              double cAnalytic{
                  initialAnalyticConcentration(globalPos[0], globalPos[1]) /
                  volOverL3};
              double norm{cAnalytic + tiffULP};
              double cFunc = gfFunc(globalPos);
              //            double cTiff = gfTiff(globalPos);
              //            double diffAnalyticTiff{std::abs(cTiff - cAnalytic)
              //            / norm};
              double diffAnalyticFunc{std::abs(cFunc - cAnalytic) / norm};
              //            double diffTiffFunc{std::abs(cTiff - cFunc) / norm};
              //            avgDiffAnalyticTiff += diffAnalyticTiff;
              avgDiffAnalyticFunc += diffAnalyticFunc;
              //            avgDiffTiffFunc += diffTiffFunc;
              n += 1.0;
            }
          }
        }
        avgDiffAnalyticTiff /= n;
        avgDiffAnalyticFunc /= n;
        avgDiffTiffFunc /= n;
        avgDiffs.push_back(
            {avgDiffAnalyticTiff, avgDiffAnalyticFunc, avgDiffTiffFunc});
      }
    }
  }
  return avgDiffs;
}

TEST_CASE("DUNE: function - large triangles",
          "[core/simulate/dunefunction][core/simulate][core][dunefunction]") {
  for (auto exampleModel : {Mod::ABtoC, Mod::VerySimpleModel}) {
    CAPTURE(exampleModel);
    auto avgDiffs{getAvgDiffs(exampleModel, 30)};
    for (const auto &avgDiff : avgDiffs) {
      // Differences between analytic expr and values due to:
      //  - Dune takes vertex values & linearly interpolates other points
      //  - Vertex values themselves are taken from nearest pixel
      //  - TIFF also has smaller ULP: ~ |max conc| / 2^16
      REQUIRE(avgDiff.AnalyticTiff < 0.01);
      REQUIRE(avgDiff.AnalyticFunc < 0.01);
      // TIFF and Func should agree better, but they can differ beyond ULP
      // issues, if for a pixel-corner
      // vertex they end up using different (equally valid) nearest pixels
      REQUIRE(avgDiff.TiffFunc < 0.001);
    }
  }
}

TEST_CASE("DUNE: function - more models, small triangles",
          "[core/simulate/dunefunction][core/"
          "simulate][core][dunefunction][expensive]") {
  for (auto exampleModel : {Mod::ABtoC, Mod::VerySimpleModel,
                            Mod::LiverSimplified, Mod::LiverCells}) {
    CAPTURE(exampleModel);
    auto avgDiffs{getAvgDiffs(exampleModel, 2)};
    for (const auto &avgDiff : avgDiffs) {
      // Differences between analytic expr and values due to:
      //  - Dune takes vertex values & linearly interpolates other points
      //  - Vertex values themselves are taken from nearest pixel
      //  - TIFF also has smaller ULP: ~ |max conc| / 2^16
      REQUIRE(avgDiff.AnalyticTiff < 0.006);
      REQUIRE(avgDiff.AnalyticFunc < 0.006);
      // TIFF and Func should agree better, but they can differ beyond ULP
      // issues, if for a pixel-corner
      // vertex they end up using different (equally valid) nearest pixels
      REQUIRE(avgDiff.TiffFunc < 0.0004);
    }
  }
}

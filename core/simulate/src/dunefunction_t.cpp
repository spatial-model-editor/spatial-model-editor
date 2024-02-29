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

using HostGrid2d = Dune::UGGrid<2>;
using MDGTraits2d = Dune::mdgrid::FewSubDomainsTraits<2, 64>;
using Grid2d = Dune::mdgrid::MultiDomainGrid<HostGrid2d, MDGTraits2d>;
using Model2d =
    Dune::Copasi::Model<Grid2d, Grid2d::SubDomainGrid::Traits::LeafGridView,
                        double, double>;

using HostGrid3d = Dune::UGGrid<3>;
using MDGTraits3d = Dune::mdgrid::FewSubDomainsTraits<3, 64>;
using Grid3d = Dune::mdgrid::MultiDomainGrid<HostGrid3d, MDGTraits3d>;
using Model3d =
    Dune::Copasi::Model<Grid3d, Grid3d::SubDomainGrid::Traits::LeafGridView,
                        double, double>;

static Dune::ParameterTree getConfig(const simulate::DuneConverter &dc) {
  Dune::ParameterTree config;
  std::stringstream ssIni(dc.getIniFile().toStdString());
  Dune::ParameterTreeParser::readINITree(ssIni, config);
  return config;
}

static double initialAnalyticConcentration(double x, double y, double z) {
  return std::sqrt(1.0 + x * x + y * y + z * z);
}

struct AvgDiff {
  double AnalyticTiff;
  double AnalyticFunc;
  double TiffFunc;
};

static void setAnalyticInitialConc(sme::model::Model &m) {
  // set initial concentration from analytic expression
  for (const auto &compId : m.getCompartments().getIds()) {
    for (const auto &id : m.getSpecies().getIds(compId)) {
      if (!m.getSpecies().getIsConstant(id)) {
        m.getSpecies().setAnalyticConcentration(id,
                                                "sqrt(1.0 + x*x + y*y + z*z)");
      }
    }
  }
}

static std::vector<AvgDiff> getAvgDiffs2d(Mod exampleModel,
                                          std::size_t maxTriangleArea = 5) {
  std::vector<AvgDiff> avgDiffs;
  auto m{getExampleModel(exampleModel)};
  setAnalyticInitialConc(m);
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
  auto [grid, hostGrid] = simulate::makeDuneGrid<HostGrid2d, MDGTraits2d>(mesh);
  auto config{getConfig(dc)};
  // make a parser
  auto parser_context{std::make_shared<Dune::Copasi::ParserContext>(
      config.sub("parser_context"))};
  auto parser_type{Dune::Copasi::string2parser.at(
      config.get("model.parser_type", Dune::Copasi::default_parser_str))};
  auto functor_factory{std::make_shared<Dune::Copasi::FunctorFactoryParser<2>>(
      parser_type, parser_context)};
  auto model{
      Dune::Copasi::make_model<Model2d>(config.sub("model"), functor_factory)};
  auto initial_state{model->make_state(grid, config.sub("model"))};
  model->interpolate(
      *initial_state,
      simulate::makeModelDuneFunctions<Model2d::Grid, Model2d::GridFunction>(
          dc, *grid));

  // create model using TIFF files for initial conditions
  auto filename = QString("tmp_gridfunction_model_%1_maxtrianglearea_%2")
                      .arg(static_cast<int>(exampleModel))
                      .arg(maxTriangleArea);
  simulate::DuneConverter dcTiff(m, {}, true, filename + ".ini");
  auto configTiff = getConfig(dcTiff);
  auto parser_context_tiff{std::make_shared<Dune::Copasi::ParserContext>(
      configTiff.sub("parser_context"))};
  auto gridTiff =
      make_multi_domain_grid<Grid2d>(configTiff, parser_context_tiff);
  // this is what the dune-copasi binary does after making the grid:
  configTiff.sub("model.compartments") = configTiff.sub("compartments");
  // make a parser
  auto parser_type_tiff{Dune::Copasi::string2parser.at(
      configTiff.get("model.parser_type", Dune::Copasi::default_parser_str))};
  auto functor_factory_tiff{
      std::make_shared<Dune::Copasi::FunctorFactoryParser<2>>(
          parser_type_tiff, parser_context_tiff)};
  auto modelTiff{Dune::Copasi::make_model<Model2d>(configTiff.sub("model"),
                                                   functor_factory_tiff)};
  auto initial_state_tiff{
      modelTiff->make_state(std::move(gridTiff), configTiff.sub("model"))};
  modelTiff->interpolate(*initial_state_tiff,
                         modelTiff->make_initial(*(initial_state_tiff->grid),
                                                 configTiff.sub("model")));

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
        auto localGfFunc = localFunction(gfFunc);
        auto gfTiff =
            modelTiff->make_compartment_function(*initial_state_tiff, species);
        auto localGfTiff = localFunction(gfTiff);
        double avgDiffAnalyticTiff{0.0};
        double avgDiffAnalyticFunc{0.0};
        double avgDiffTiffFunc{0.0};
        double n{0.0};
        for (const auto &e : elements(
                 grid->subDomain(static_cast<int>(domain)).leafGridView())) {
          localGfFunc.bind(e);
          localGfTiff.bind(e);
          for (double x : {0.0}) {
            for (double y : {0.0}) {
              Dune::FieldVector<double, 2> local{x, y};
              auto globalPos = e.geometry().global(local);
              double cAnalytic{
                  initialAnalyticConcentration(globalPos[0], globalPos[1], 0) /
                  volOverL3};
              double norm{cAnalytic + tiffULP};
              double cFunc = localGfFunc(local);
              double cTiff = localGfTiff(local);
              double diffAnalyticTiff{std::abs(cTiff - cAnalytic) / norm};
              double diffAnalyticFunc{std::abs(cFunc - cAnalytic) / norm};
              double diffTiffFunc{std::abs(cTiff - cFunc) / norm};
              avgDiffAnalyticTiff += diffAnalyticTiff;
              avgDiffAnalyticFunc += diffAnalyticFunc;
              avgDiffTiffFunc += diffTiffFunc;
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

static std::vector<double> getAvgDiffs3d(Mod exampleModel,
                                         std::size_t maxCellVolume) {
  std::vector<double> avgDiffs;
  auto m{getExampleModel(exampleModel)};
  setAnalyticInitialConc(m);
  auto nCompartments{m.getCompartments().getIds().size()};
  auto &mesh3d{*(m.getGeometry().getMesh3d())};
  for (std::size_t i = 0; i < nCompartments; ++i) {
    mesh3d.setCompartmentMaxCellVolume(i, maxCellVolume);
  }

  const auto &lengthUnit = m.getUnits().getLength();
  const auto &volumeUnit = m.getUnits().getVolume();
  double volOverL3{model::getVolOverL3(lengthUnit, volumeUnit)};

  // create model using GridFunction for initial conditions
  simulate::DuneConverter dc(m, {}, false);
  auto [grid, hostGrid] =
      simulate::makeDuneGrid<HostGrid3d, MDGTraits3d>(mesh3d);
  auto config{getConfig(dc)};
  // make a parser
  auto parser_context{std::make_shared<Dune::Copasi::ParserContext>(
      config.sub("parser_context"))};
  auto parser_type{Dune::Copasi::string2parser.at(
      config.get("model.parser_type", Dune::Copasi::default_parser_str))};
  auto functor_factory{std::make_shared<Dune::Copasi::FunctorFactoryParser<3>>(
      parser_type, parser_context)};
  auto model{
      Dune::Copasi::make_model<Model3d>(config.sub("model"), functor_factory)};
  auto initial_state{model->make_state(grid, config.sub("model"))};
  model->interpolate(
      *initial_state,
      simulate::makeModelDuneFunctions<Model3d::Grid, Model3d::GridFunction>(
          dc, *grid));

  // todo: when dune-copasi supports 3d tiff initial conditions, also compare
  // with that as done above in the 2d case

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
        auto localGfFunc = localFunction(gfFunc);
        double avgDiffAnalyticFunc{0.0};
        double n{0.0};
        for (const auto &e : elements(
                 grid->subDomain(static_cast<int>(domain)).leafGridView())) {
          localGfFunc.bind(e);
          for (double x : {0.5}) {
            for (double y : {0.5}) {
              for (double z : {0.5}) {
                Dune::FieldVector<double, 3> local{x, y, z};
                auto globalPos = e.geometry().global(local);
                double cAnalytic{initialAnalyticConcentration(
                                     globalPos[0], globalPos[1], globalPos[2]) /
                                 volOverL3};
                double norm{cAnalytic + tiffULP};
                double cFunc = localGfFunc(local);
                double diffAnalyticFunc{std::abs(cFunc - cAnalytic) / norm};
                avgDiffAnalyticFunc += diffAnalyticFunc;
                n += 1.0;
              }
            }
          }
        }
        avgDiffAnalyticFunc /= n;
        avgDiffs.push_back(avgDiffAnalyticFunc);
      }
    }
  }
  return avgDiffs;
}

TEST_CASE("DUNE: function 2d - large triangles",
          "[core/simulate/dunefunction][core/"
          "simulate][core][dunefunction][dune][2d]") {
  for (auto exampleModel : {Mod::ABtoC, Mod::VerySimpleModel}) {
    CAPTURE(exampleModel);
    auto avgDiffs{getAvgDiffs2d(exampleModel, 30)};
    for (const auto &avgDiff : avgDiffs) {
      // Differences between analytic expr and values due to:
      //  - Dune takes vertex values & linearly interpolates other points
      //  - Vertex values themselves are taken from nearest pixel
      //  - TIFF also has smaller ULP: ~ |max conc| / 2^16
      REQUIRE(avgDiff.AnalyticTiff < 0.060);
      REQUIRE(avgDiff.AnalyticFunc < 0.010);
      // TIFF and Func should agree better, but they can differ beyond ULP
      // issues, if for a pixel-corner
      // vertex they end up using different (equally valid) nearest pixels
      REQUIRE(avgDiff.TiffFunc < 0.050);
    }
  }
}

TEST_CASE("DUNE: function 2d - more models, small triangles",
          "[core/simulate/dunefunction][core/"
          "simulate][core][dunefunction][expensive][dune][2d]") {
  for (auto exampleModel : {Mod::ABtoC, Mod::VerySimpleModel,
                            Mod::LiverSimplified, Mod::LiverCells}) {
    CAPTURE(exampleModel);
    auto avgDiffs{getAvgDiffs2d(exampleModel, 2)};
    for (const auto &avgDiff : avgDiffs) {
      // Differences between analytic expr and values due to:
      //  - Dune takes vertex values & linearly interpolates other points
      //  - Vertex values themselves are taken from nearest pixel
      //  - TIFF also has smaller ULP: ~ |max conc| / 2^16
      REQUIRE(avgDiff.AnalyticTiff < 0.025);
      REQUIRE(avgDiff.AnalyticFunc < 0.006);
      // TIFF and Func should agree better, but they can differ beyond ULP
      // issues, if for a pixel-corner
      // vertex they end up using different (equally valid) nearest pixels
      REQUIRE(avgDiff.TiffFunc < 0.020);
    }
  }
}

TEST_CASE("DUNE: function 3d", "[core/simulate/dunefunction][core/"
                               "simulate][core][dunefunction][dune][3d]") {
  for (auto exampleModel :
       {Mod::SingleCompartmentDiffusion3D, Mod::VerySimpleModel3D}) {
    CAPTURE(exampleModel);
    auto avgDiffs = getAvgDiffs3d(exampleModel, 12);
    CAPTURE(avgDiffs.size());
    for (const auto &avgDiff : avgDiffs) {
      // Differences between analytic expr and values due to:
      //  - Dune takes vertex values & linearly interpolates other points
      //  - Vertex values themselves are taken from nearest pixel
      //  - Some vertices lie well outside the actual compartment due to meshing
      REQUIRE(avgDiff < 0.100);
    }
  }
}

TEST_CASE("DUNE: function 3d - small cell volumes",
          "[core/simulate/dunefunction][core/"
          "simulate][expensive][core][dunefunction][dune][3d]") {
  for (auto exampleModel :
       {Mod::SingleCompartmentDiffusion3D, Mod::VerySimpleModel3D}) {
    CAPTURE(exampleModel);
    auto avgDiffs = getAvgDiffs3d(exampleModel, 4);
    CAPTURE(avgDiffs.size());
    for (const auto &avgDiff : avgDiffs) {
      REQUIRE(avgDiff < 0.020);
    }
  }
}

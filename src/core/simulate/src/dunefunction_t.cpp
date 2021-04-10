#include "catch_wrapper.hpp"
#include "dune_headers.hpp"
#include "duneconverter.hpp"
#include "dunefunction.hpp"
#include "dunegrid.hpp"
#include "model.hpp"
#include <QFile>
#include <cmath>
#include <locale>

using namespace sme;

constexpr int DuneDimensions = 2;
using HostGrid = Dune::UGGrid<DuneDimensions>;
using MDGTraits = Dune::mdgrid::DynamicSubDomainCountTraits<DuneDimensions, 1>;
using Grid = Dune::mdgrid::MultiDomainGrid<HostGrid, MDGTraits>;
using ModelTraits =
    Dune::Copasi::ModelMultiDomainP0PkDiffusionReactionTraits<Grid, 1>;
using Model = Dune::Copasi::ModelMultiDomainDiffusionReaction<ModelTraits>;

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

SCENARIO("DUNE: function",
         "[core/simulate/dunefunction][core/simulate][core][dunefunction]") {
  // init & mute Dune logging
  if (!Dune::Logging::Logging::initialized()) {
    Dune::Logging::Logging::init(
        Dune::FakeMPIHelper::getCollectiveCommunication());
  }
  Dune::Logging::Logging::mute();

  for (const auto &modelName :
       {"ABtoC", "very-simple-model", "liver-simplified", "liver-cells"}) {
    CAPTURE(modelName);
    model::Model m;
    if (QFile f(QString(":/models/%1.xml").arg(modelName));
        f.open(QIODevice::ReadOnly)) {
      m.importSBMLString(f.readAll().toStdString());
    }
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
      mesh.setCompartmentMaxTriangleArea(i, 5);
    }

    const auto &lengthUnit = m.getUnits().getLength();
    const auto &volumeUnit = m.getUnits().getVolume();
    double volOverL3{model::getVolOverL3(lengthUnit, volumeUnit)};

    // create model using GridFunction for initial conditions
    auto stages =
        Dune::Copasi::BitFlags<Dune::Copasi::ModelSetup::Stages>::all_flags();
    stages.reset(Dune::Copasi::ModelSetup::Stages::Writer);
    simulate::DuneConverter dc(m, false);
    auto [grid, hostGrid] = simulate::makeDuneGrid<HostGrid, MDGTraits>(mesh);
    auto config = getConfig(dc);
    Model model(grid, config.sub("model"), stages);
    model.set_initial(simulate::makeModelDuneFunctions<Grid::LeafGridView>(dc));

    // create model using TIFF files for initial conditions
    simulate::DuneConverter dcTiff(m, true);
    auto configTiff = getConfig(dcTiff);
    Model modelTiff(grid, configTiff.sub("model"));

    // compare initial species concentrations
    for (int domain = 0; domain < nCompartments; ++domain) {
      const auto &c{m.getSpecies().getSampledFieldConcentration(
          m.getSpecies().getIds(m.getCompartments().getIds()[domain])[0])};
      double tiffULP{*std::max_element(c.cbegin(), c.cend()) / 65536.0 /
                     volOverL3};
      auto gfFunc =
          model.get_grid_function(static_cast<std::size_t>(domain), 0);
      auto gfTiff =
          modelTiff.get_grid_function(static_cast<std::size_t>(domain), 0);
      double avgDiffAnalyticTiff{0.0};
      double avgDiffAnalyticFunc{0.0};
      double avgDiffTiffFunc{0.0};
      double n{0.0};
      for (const auto &e : elements(grid->subDomain(domain).leafGridView())) {
        Dune::FieldVector<double, 1> cFunc;
        Dune::FieldVector<double, 1> cTiff;
        for (double x : {0.0}) {
          for (double y : {0.0}) {
            Dune::FieldVector<double, 2> local{x, y};
            auto globalPos = e.geometry().global(local);
            double cAnalytic{
                initialAnalyticConcentration(globalPos[0], globalPos[1]) /
                volOverL3};
            double norm{cAnalytic + tiffULP};
            gfFunc->evaluate(e, local, cFunc);
            gfTiff->evaluate(e, local, cTiff);
            double diffAnalyticTiff{std::abs(cTiff[0] - cAnalytic) / norm};
            double diffAnalyticFunc{std::abs(cFunc[0] - cAnalytic) / norm};
            double diffTiffFunc{std::abs(cTiff[0] - cFunc[0]) / norm};
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
      CAPTURE(domain);
      // Differences between analytic expr and values due to:
      //  - Dune takes vertex values & linearly interpolates other points
      //  - Vertex values themselves are taken from nearest pixel
      //  - TIFF also has smaller ULP: ~ |max conc| / 2^16
      REQUIRE(avgDiffAnalyticTiff < 0.08);
      REQUIRE(avgDiffAnalyticFunc < 0.08);
      // TIFF and Func can differ beyond ULP issues, if for a pixel-corner
      // vertex they end up using different (equally valid) nearest pixels
      REQUIRE(avgDiffTiffFunc < 0.008);
    }
  }
}

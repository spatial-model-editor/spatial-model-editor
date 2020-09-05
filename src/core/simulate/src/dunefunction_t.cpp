#include "catch_wrapper.hpp"
#include "dune_headers.hpp"
#include "duneconverter.hpp"
#include "dunefunction.hpp"
#include "dunegrid.hpp"
#include "model.hpp"
#include <QFile>
#include <locale>

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

SCENARIO("DUNE: function",
         "[core/simulate/dunefunction][core/simulate][core][dunefunction]") {
  // init & mute Dune logging
  if (!Dune::Logging::Logging::initialized()) {
    Dune::Logging::Logging::init(
        Dune::FakeMPIHelper::getCollectiveCommunication());
  }
  Dune::Logging::Logging::mute();

  for (const auto &modelName : {"very-simple-model"}) {
    CAPTURE(modelName);

    model::Model m;
    if (QFile f(QString(":/models/%1.xml").arg(modelName));
        f.open(QIODevice::ReadOnly)) {
      m.importSBMLString(f.readAll().toStdString());
    }
    for (const auto &compId : m.getCompartments().getIds()) {
      for (const auto &id : m.getSpecies().getIds(compId)) {
        if (!m.getSpecies().getIsConstant(id)) {
          m.getSpecies().setAnalyticConcentration(id, "cos(x/5.0)+1.1");
        }
      }
    }
    // create model using functions for initial conditions
    auto stages =
        Dune::Copasi::BitFlags<Dune::Copasi::ModelSetup::Stages>::all_flags();
    stages.reset(Dune::Copasi::ModelSetup::Stages::Writer);
    simulate::DuneConverter dc(m, false);
    auto [grid, hostGrid] = simulate::makeDuneGrid<HostGrid, MDGTraits>(
        *m.getGeometry().getMesh(), dc.getGMSHCompIndices());
    auto config = getConfig(dc);
    Model model(grid, config.sub("model"), stages);
    model.set_initial(simulate::makeModelDuneFunctions<Grid::LeafGridView>(dc));

    // create model using TIFF files for initial conditions
    simulate::DuneConverter dcTiff(m, true);
    auto configTiff = getConfig(dcTiff);
    Model modelTiff(grid, configTiff.sub("model"));

    // compare initial species concentrations
    auto gf = model.get_grid_function(0, 0);
    auto gfTiff = modelTiff.get_grid_function(0, 0);

    double relativeError{0.0};
    constexpr double eps{1e-8};
    double n{0.0};
    for (const auto &e : elements(grid->subDomain(0).leafGridView())) {
      Dune::FieldVector<double, 1> c;
      Dune::FieldVector<double, 1> cTiff;
      for (double x : {0.0, 0.2, 0.5, 0.887}) {
        for (double y : {0.0, 0.1, 0.5, 0.99}) {
          Dune::FieldVector<double, 2> local{x, y};
          gf->evaluate(e, local, c);
          gfTiff->evaluate(e, local, cTiff);
          relativeError +=
              std::abs(c[0] - cTiff[0]) /
              (0.5 * std::abs(c[0]) + 0.5 * std::abs(cTiff[0]) + eps);
          n += 1.0;
        }
      }
    }
    double avgRelErr{relativeError / n};
    // Note: due to boundary approx in mesh, sometimes the nearest pixel to a
    // Dune point is in another compartment. In the GUI we correct for this by
    // using the nearest pixel in the same compartment, but when dune loads a
    // TIFF it will just use the pixel from the wrong compartment (which will
    // return zero for the concentration) instead.
    // Hence the requirement for agreement within 10% on average:
    REQUIRE(avgRelErr < 0.1);
  }
}

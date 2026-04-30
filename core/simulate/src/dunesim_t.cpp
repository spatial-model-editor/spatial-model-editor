#include "catch_wrapper.hpp"
#include "dune_headers.hpp"
#include "dunegrid.hpp"
#include "dunesim.hpp"
#include "model_test_utils.hpp"
#include "sme/mesh2d.hpp"
#include "sme/model.hpp"
#include "sme/simulate_options.hpp"
#include "sme/utils.hpp"
#include <array>
#include <locale>
#include <optional>

using namespace sme;
using namespace sme::test;

namespace {

class ScopedTestLocale {
public:
  explicit ScopedTestLocale(std::locale newLocale)
      : previousLocale(std::locale::global(std::move(newLocale))) {}
  ~ScopedTestLocale() { std::locale::global(previousLocale); }

private:
  std::locale previousLocale;
};

std::optional<std::locale> getGermanLocale() {
  for (const char *localeName : {"de_DE.UTF-8", "de_DE.utf8", "de_DE"}) {
    try {
      return std::locale(localeName);
    } catch (const std::runtime_error &) {
    }
  }
  return std::nullopt;
}

using HostGrid2d = Dune::UGGrid<2>;
using MDGTraits2d = Dune::mdgrid::FewSubDomainsTraits<2, 64>;

template <typename Grid>
std::vector<double> getAllElementCoordinates(Grid *grid,
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

std::array<std::vector<double>, 2>
getSubdomainElementCoordinates(const mesh::Mesh2d &mesh) {
  auto duneGrid = simulate::makeDuneGrid<HostGrid2d, MDGTraits2d>(mesh);
  return {getAllElementCoordinates(duneGrid.first.get(), 0),
          getAllElementCoordinates(duneGrid.first.get(), 1)};
}

} // namespace

TEST_CASE("DuneSim", "[core/simulate/dunesim][core/"
                     "simulate][core][simulate][dunesim][dune]") {
  SECTION("Model has no species") {
    auto m{getExampleModel(Mod::ABtoC)};
    for (const auto &speciesId : m.getSpecies().getIds("comp")) {
      m.getSpecies().remove(speciesId);
    }
    std::vector<std::string> comps{"comp"};
    simulate::DuneSim duneSim(m, comps);
    REQUIRE(duneSim.errorMessage().empty());
    duneSim.run(0.05, 100e3, {});
    REQUIRE(duneSim.errorMessage().empty());
  }
  SECTION(
      "Compartment in model has no species, but membrane contains reactions") {
    // see
    // https://github.com/spatial-model-editor/spatial-model-editor/issues/435
    auto m{getExampleModel(Mod::VerySimpleModel)};
    m.getSpecies().setInitialConcentration("B_c2", 0.1);
    auto col1{m.getCompartments().getColor("c1")};
    auto col2{m.getCompartments().getColor("c2")};
    auto &duneOptions{m.getSimulationSettings().options.dune};
    duneOptions.maxThreads = 1;
    duneOptions.dt = 0.05;
    duneOptions.minDt = 0.05;
    duneOptions.maxDt = 0.05;
    duneOptions.newtonRelErr = 1e-14;
    duneOptions.newtonAbsErr = 1e-14;
    duneOptions.linearSolver = "BiCGSTAB";
    // remove all species from nucleus, but keep compartment in model
    m.getSpecies().remove("A_c3");
    m.getSpecies().remove("B_c3");
    // re-assign compartment color to generate default boundaries & mesh
    m.getCompartments().setColor("c1", col1);
    std::vector<std::string> comps{"c1", "c2", "c3"};
    auto maxPoints{m.getGeometry().getMesh2d()->getBoundaryMaxPoints()};
    auto maxAreas{m.getGeometry().getMesh2d()->getCompartmentMaxTriangleArea()};
    // ensure max triangle areas for compartment that we will later remove don't
    // introduce any steiner points
    m.getGeometry().getMesh2d()->setCompartmentMaxTriangleArea(2, 9999);
    REQUIRE(maxPoints.size() == 3);
    REQUIRE(maxAreas.size() == 3);
    const auto *meshWithEmptyCompartment{m.getGeometry().getMesh2d()};
    REQUIRE(meshWithEmptyCompartment != nullptr);
    REQUIRE(meshWithEmptyCompartment->isValid());
    const std::array<std::size_t, 2> triangleCountsWithEmptyCompartment{
        meshWithEmptyCompartment->getTriangleIndices()[0].size(),
        meshWithEmptyCompartment->getTriangleIndices()[1].size()};
    const auto elementCoordinatesWithEmptyCompartment{
        getSubdomainElementCoordinates(*meshWithEmptyCompartment)};
    simulate::DuneSim duneSim(m, comps);
    for (std::size_t i = 0; i < 2; ++i) {
      duneSim.run(0.05, 100e3, {});
    }
    simulate::SimulationData data0{m.getSimulationData()};
    REQUIRE(duneSim.getConcentrations(0).size() == 5441);
    REQUIRE(duneSim.getConcentrations(1).size() == 8068);
    REQUIRE(duneSim.errorMessage().empty());
    // completely remove nucleus compartment from model and repeat:
    // shouldn't change simulation results
    m.getCompartments().remove("c3");
    m.getCompartments().setColor("c1", col1);
    m.getCompartments().setColor("c2", col2);
    REQUIRE(m.getGeometry().getMesh2d()->getBoundaryMaxPoints().size() == 3);
    REQUIRE(
        m.getGeometry().getMesh2d()->getCompartmentMaxTriangleArea().size() ==
        2);
    m.getGeometry().getMesh2d()->setBoundaryMaxPoints(0, maxPoints[0]);
    m.getGeometry().getMesh2d()->setBoundaryMaxPoints(1, maxPoints[1]);
    m.getGeometry().getMesh2d()->setBoundaryMaxPoints(2, maxPoints[2]);
    m.getGeometry().getMesh2d()->setCompartmentMaxTriangleArea(0, maxAreas[0]);
    m.getGeometry().getMesh2d()->setCompartmentMaxTriangleArea(1, maxAreas[1]);
    const auto *meshWithoutEmptyCompartment{m.getGeometry().getMesh2d()};
    REQUIRE(meshWithoutEmptyCompartment != nullptr);
    REQUIRE(meshWithoutEmptyCompartment->isValid());
    const auto &triangleIndicesWithoutEmptyCompartment{
        meshWithoutEmptyCompartment->getTriangleIndices()};
    REQUIRE(triangleIndicesWithoutEmptyCompartment.size() == 2);
    const auto elementCoordinatesWithoutEmptyCompartment{
        getSubdomainElementCoordinates(*meshWithoutEmptyCompartment)};
    for (std::size_t iComp = 0; iComp < 2; ++iComp) {
      CAPTURE(iComp);
      REQUIRE(triangleCountsWithEmptyCompartment[iComp] ==
              triangleIndicesWithoutEmptyCompartment[iComp].size());
      REQUIRE(elementCoordinatesWithEmptyCompartment[iComp].size() ==
              elementCoordinatesWithoutEmptyCompartment[iComp].size());
      for (std::size_t i = 0;
           i < elementCoordinatesWithEmptyCompartment[iComp].size(); ++i) {
        CAPTURE(i);
        REQUIRE(
            elementCoordinatesWithEmptyCompartment[iComp][i] ==
            dbl_approx(elementCoordinatesWithoutEmptyCompartment[iComp][i]));
      }
    }
    comps.pop_back();
    simulate::DuneSim newDuneSim(m, comps);
    for (std::size_t i = 0; i < 2; ++i) {
      newDuneSim.run(0.05, 100e3, {});
    }
    REQUIRE(newDuneSim.getConcentrations(0).size() == 5441);
    REQUIRE(newDuneSim.getConcentrations(1).size() == 8068);
    for (std::size_t iComp = 0; iComp < 2; ++iComp) {
      double diff{0};
      double sum{0};
      const auto n{duneSim.getConcentrations(iComp).size()};
      const auto &a{duneSim.getConcentrations(iComp)};
      const auto &b{newDuneSim.getConcentrations(iComp)};
      for (std::size_t i = 0; i < n; ++i) {
        diff += std::abs(a[i] - b[i]);
        sum += std::abs(a[i]) + std::abs(b[i]);
      }
      CAPTURE(sum);
      CAPTURE(diff);
      REQUIRE(diff / sum < 1e-13);
    }
  }
  SECTION("Callback is provided and used to stop simulation") {
    auto m{getExampleModel(Mod::ABtoC)};
    std::vector<std::string> comps{"comp"};
    simulate::DuneSim duneSim(m, comps);
    REQUIRE(duneSim.errorMessage().empty());
    duneSim.run(1, -1, []() { return true; });
    REQUIRE(duneSim.errorMessage() == "Simulation cancelled");
  }
  SECTION("Species are mapped to the correct initial concentrations") {
    // https://github.com/spatial-model-editor/spatial-model-editor/issues/852
    // used inverse mapping of indices, which happened to be correct for test
    // models:
    {
      auto m{getExampleModel(Mod::ABtoC)};
      m.getSpecies().setInitialConcentration("A", 1.0);
      m.getSpecies().setInitialConcentration("B", 2.0);
      m.getSpecies().setInitialConcentration("C", 3.0);
      std::vector<std::string> comps{"comp"};
      simulate::DuneSim duneSim(m, comps);
      REQUIRE(duneSim.errorMessage().empty());
      REQUIRE(m.getSpecies().getIds("comp")[0] == "A");
      REQUIRE(duneSim.getConcentrations(0)[0] == dbl_approx(1.0));
      REQUIRE(m.getSpecies().getIds("comp")[1] == "B");
      REQUIRE(duneSim.getConcentrations(0)[1] == dbl_approx(2.0));
      REQUIRE(m.getSpecies().getIds("comp")[2] == "C");
      REQUIRE(duneSim.getConcentrations(0)[2] == dbl_approx(3.0));
    }
    // but for this variant with ids X,Y,W the inverse mapping is not the same:
    {
      auto m{getTestModel("XYtoW")};
      m.getSpecies().setInitialConcentration("X", 5.0);
      m.getSpecies().setInitialConcentration("Y", 2.0);
      m.getSpecies().setInitialConcentration("W", 3.0);
      std::vector<std::string> comps{"comp"};
      simulate::DuneSim duneSim(m, comps);
      REQUIRE(duneSim.errorMessage().empty());
      REQUIRE(m.getSpecies().getIds("comp")[0] == "X");
      REQUIRE(duneSim.getConcentrations(0)[0] == dbl_approx(5.0));
      REQUIRE(m.getSpecies().getIds("comp")[1] == "Y");
      REQUIRE(duneSim.getConcentrations(0)[1] == dbl_approx(2.0));
      REQUIRE(m.getSpecies().getIds("comp")[2] == "W");
      REQUIRE(duneSim.getConcentrations(0)[2] == dbl_approx(3.0));
    }
  }
  SECTION("Construction is locale-independent") {
    auto deLocale = getGermanLocale();
    if (!deLocale.has_value()) {
      WARN("Skipping locale test, no DE locale available");
      return;
    }
    ScopedTestLocale localeGuard(*deLocale);

    auto m{getTestModel("txy")};
    m.getSpecies().remove("A");
    m.getSpecies().remove("B");
    m.getSimulationSettings().options.dune.dt = 1e-3;

    const auto comps = common::toStdString(m.getCompartments().getIds());
    simulate::DuneSim duneSim(m, comps);
    REQUIRE(duneSim.errorMessage().empty());
    duneSim.run(1e-3, 100e3, {});
    REQUIRE(duneSim.errorMessage().empty());
  }
}

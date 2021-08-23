#include "catch_wrapper.hpp"
#include "dunesim.hpp"
#include "model.hpp"
#include "model_test_utils.hpp"

using namespace sme;
using namespace sme::test;

SCENARIO("DuneSim: empty compartments",
         "[core/simulate/dunesim][core/"
         "simulate][core][simulate][dunesim][dune]") {
  WHEN("Model has no species") {
    auto m{getExampleModel(Mod::ABtoC)};
    for (const auto &speciesId : m.getSpecies().getIds("comp")) {
      m.getSpecies().remove(speciesId);
    }
    std::vector<std::string> comps{"comp"};
    simulate::DuneSim duneSim(m, comps);
    REQUIRE(duneSim.errorMessage().empty());
  }
  WHEN("Compartment in model has no species, but membrane contains reactions") {
    // see
    // https://github.com/spatial-model-editor/spatial-model-editor/issues/435
    auto m{getExampleModel(Mod::VerySimpleModel)};
    m.getSpecies().setInitialConcentration("B_c2", 0.1);
    auto col1{m.getCompartments().getColour("c1")};
    auto col2{m.getCompartments().getColour("c2")};
    // remove all species from nucleus, but keep compartment in model
    m.getSpecies().remove("A_c3");
    m.getSpecies().remove("B_c3");
    // re-assign compartment colour to generate default boundaries & mesh
    m.getCompartments().setColour("c1", col1);
    std::vector<std::string> comps{"c1", "c2", "c3"};
    auto maxPoints{m.getGeometry().getMesh()->getBoundaryMaxPoints()};
    auto maxAreas{m.getGeometry().getMesh()->getCompartmentMaxTriangleArea()};
    // ensure max triangle areas for compartment that we will later remove don't
    // introduce any steiner points
    m.getGeometry().getMesh()->setCompartmentMaxTriangleArea(2, 9999);
    REQUIRE(maxPoints.size() == 3);
    REQUIRE(maxAreas.size() == 3);
    simulate::DuneSim duneSim(m, comps);
    for (std::size_t i = 0; i < 2; ++i) {
      duneSim.run(0.05, 100e3);
    }
    simulate::SimulationData data0{m.getSimulationData()};
    REQUIRE(duneSim.getConcentrations(0).size() == 5441);
    REQUIRE(duneSim.getConcentrations(1).size() == 8068);
    REQUIRE(duneSim.errorMessage().empty());
    // completely remove nucleus compartment from model and repeat:
    // shouldn't change simulation results
    m.getCompartments().remove("c3");
    m.getCompartments().setColour("c1", col1);
    m.getCompartments().setColour("c2", col2);
    REQUIRE(m.getGeometry().getMesh()->getBoundaryMaxPoints().size() == 3);
    REQUIRE(m.getGeometry().getMesh()->getCompartmentMaxTriangleArea().size() ==
            2);
    m.getGeometry().getMesh()->setBoundaryMaxPoints(0, maxPoints[0]);
    m.getGeometry().getMesh()->setBoundaryMaxPoints(1, maxPoints[1]);
    m.getGeometry().getMesh()->setBoundaryMaxPoints(2, maxPoints[2]);
    m.getGeometry().getMesh()->setCompartmentMaxTriangleArea(0, maxAreas[0]);
    m.getGeometry().getMesh()->setCompartmentMaxTriangleArea(1, maxAreas[1]);
    comps.pop_back();
    simulate::DuneSim newDuneSim(m, comps);
    for (std::size_t i = 0; i < 2; ++i) {
      newDuneSim.run(0.05, 100e3);
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
      REQUIRE(diff / sum < 1e-10);
    }
  }
}

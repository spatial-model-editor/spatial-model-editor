#include "catch_wrapper.hpp"
#include "dunesim.hpp"
#include "model.hpp"
#include "utils.hpp"
#include <QFile>

SCENARIO("DuneSim: model without species",
         "[core/simulate/dunesim][core/simulate][core][simulate][dunesim]") {
  model::Model m;
  QFile f(":/models/ABtoC.xml");
  f.open(QIODevice::ReadOnly);
  m.importSBMLString(f.readAll().toStdString());
  for (const auto &speciesId : m.getSpecies().getIds("comp")) {
    m.getSpecies().remove(speciesId);
  }
  std::vector<std::string> comps{"comp"};
  std::vector<std::vector<std::string>> species;
  species.emplace_back();
  simulate::DuneSim duneSim(m, comps, species);
  REQUIRE(duneSim.errorMessage() ==
          "Nothing to simulate: no non-constant species in model");
}

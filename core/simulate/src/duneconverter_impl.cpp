#include "duneconverter_impl.hpp"
#include "duneini.hpp"
#include "sme/geometry.hpp"
#include "sme/logger.hpp"
#include "sme/mesh.hpp"
#include "sme/model.hpp"
#include "sme/model_compartments.hpp"
#include "sme/model_geometry.hpp"
#include "sme/model_membranes.hpp"
#include "sme/model_reactions.hpp"
#include "sme/model_species.hpp"
#include "sme/model_units.hpp"
#include "sme/pde.hpp"
#include "sme/simulate_options.hpp"
#include "sme/tiff.hpp"
#include "sme/utils.hpp"
#include <QDir>
#include <QFile>
#include <algorithm>
#include <cstddef>
#include <memory>
#include <numeric>
#include <string>
#include <utility>

namespace sme::simulate {

std::vector<std::string>
makeValidDuneSpeciesNames(const std::vector<std::string> &names) {
  // todo: check for clashes in other compartments as well??
  std::vector<std::string> duneNames = names;
  // muparser reserved words, taken from:
  // https://beltoforion.de/article.php?a=muparser&p=features
  std::vector<std::string> reservedNames{
      {"sin",  "cos",  "tan",   "asin",  "acos",  "atan", "sinh",
       "cosh", "tanh", "asinh", "acosh", "atanh", "log2", "log10",
       "log",  "ln",   "exp",   "sqrt",  "sign",  "rint", "abs",
       "min",  "max",  "sum",   "avg"}};
  // dune-copasi reserved words:
  reservedNames.insert(reservedNames.end(), {"x", "y", "t", "pi", "dim"});
  for (auto &name : duneNames) {
    SPDLOG_TRACE("name {}", name);
    std::string duneName = name;
    name = "";
    // if species name clashes with a reserved name, append an underscore
    if (std::find(reservedNames.cbegin(), reservedNames.cend(), duneName) !=
        reservedNames.cend()) {
      duneName.append("_");
    }
    // if species name ends with "_o" or "_i", append an underscore
    // to avoid possible dune-copasi parsing issues with flux bcs
    if (duneName.size() > 1 && duneName[duneName.size() - 2] == '_' &&
        (duneName.back() == 'o' || duneName.back() == 'i')) {
      duneName.append("_");
    }
    // if species name clashes with another species name,
    // append another underscore
    while (std::find(duneNames.cbegin(), duneNames.cend(), duneName) !=
           duneNames.cend()) {
      duneName.append("_");
    }
    name = duneName;
    SPDLOG_TRACE("  -> {}", name);
  }
  return duneNames;
}

bool compartmentContainsNonConstantSpecies(const model::Model &model,
                                           const QString &compId) {
  const auto &specs = model.getSpecies().getIds(compId);
  return std::any_of(specs.cbegin(), specs.cend(), [m = &model](const auto &s) {
    return !m->getSpecies().getIsConstant(s);
  });
}

std::vector<std::string> getNonConstantSpecies(const model::Model &model,
                                               const QString &compId) {
  std::vector<std::string> v;
  for (const auto &s : model.getSpecies().getIds(compId)) {
    if (!model.getSpecies().getIsConstant(s)) {
      v.push_back(s.toStdString());
    }
  }
  return v;
}

bool modelHasIndependentCompartments(const model::Model &model) {
  for (const auto &memId : model.getMembranes().getIds()) {
    if (!model.getReactions().getIds(memId).isEmpty()) {
      // model has membrane reaction -> compartments coupled
      return false;
    }
  }
  return true;
}

} // namespace sme::simulate

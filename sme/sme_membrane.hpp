#pragma once

#include "sme_common.hpp"
#include "sme_reaction.hpp"
#include "sme_species.hpp"
#include <map>
#include <string>
#include <vector>

namespace model {
class Model;
}

namespace pybind11 {
class module;
}

namespace sme {

void pybindMembrane(const pybind11::module &m);

class Membrane {
private:
  model::Model *s;
  std::string id;

public:
  explicit Membrane(model::Model *sbmlDocWrapper, const std::string &sId);
  std::string getName() const;
  std::vector<Reaction> reactions;
  const Reaction &getReaction(const std::string &name) const;
  std::string getStr() const;
};

} // namespace sme

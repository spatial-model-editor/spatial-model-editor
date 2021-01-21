#pragma once

#include "sme_reactionparameter.hpp"
#include <pybind11/pybind11.h>
#include <string>
#include <vector>

namespace model {
class Model;
}

namespace sme {

void pybindReaction(pybind11::module &m);

class Reaction {
private:
  model::Model *s;
  std::string id;

public:
  Reaction() = default;
  explicit Reaction(model::Model *sbmlDocWrapper, const std::string &sId);
  std::string getName() const;
  void setName(const std::string &name);
  std::vector<ReactionParameter> parameters;
  std::string getStr() const;
};

} // namespace sme

PYBIND11_MAKE_OPAQUE(std::vector<sme::Reaction>)

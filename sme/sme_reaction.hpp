#pragma once

#include "sme_reactionparameter.hpp"
#include <string>
#include <vector>

namespace model {
class Model;
}

namespace pybind11 {
class module;
}

namespace sme {

void pybindReaction(const pybind11::module &m);

class Reaction {
private:
  model::Model *s;
  std::string id;

public:
  explicit Reaction(model::Model *sbmlDocWrapper, const std::string &sId);
  std::string getName() const;
  void setName(const std::string &name);
  std::vector<ReactionParameter> parameters;
  const ReactionParameter &getParameter(const std::string &name) const;
  std::string getStr() const;
};

} // namespace sme

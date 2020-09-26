#pragma once

#include <string>

namespace model {
class Model;
}

namespace pybind11 {
class module;
}

namespace sme {

void pybindReactionParameter(const pybind11::module &m);

class ReactionParameter {
private:
  model::Model *s;
  const std::string reacId;
  const std::string paramId;

public:
  explicit ReactionParameter(model::Model *sbmlDocWrapper,
                             const std::string &reactionId,
                             const std::string &parameterId);
  std::string getName() const;
  void setName(const std::string &name);
  double getValue() const;
  void setValue(double value);
  std::string getStr() const;
};

} // namespace sme

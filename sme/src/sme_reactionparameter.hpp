#pragma once

#include <pybind11/pybind11.h>
#include <string>

namespace model {
class Model;
}

namespace sme {

void pybindReactionParameter(pybind11::module &m);

class ReactionParameter {
private:
  model::Model *s;
  std::string reacId;
  std::string paramId;

public:
  ReactionParameter() = default;
  explicit ReactionParameter(model::Model *sbmlDocWrapper,
                             const std::string &reactionId,
                             const std::string &parameterId);
  [[nodiscard]] std::string getName() const;
  void setName(const std::string &name);
  [[nodiscard]] double getValue() const;
  void setValue(double value);
  [[nodiscard]] std::string getStr() const;
};

} // namespace sme

PYBIND11_MAKE_OPAQUE(std::vector<sme::ReactionParameter>)

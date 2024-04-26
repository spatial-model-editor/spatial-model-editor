#pragma once

#include <nanobind/nanobind.h>
#include <string>
#include <vector>

namespace sme::model {
class Model;
}

namespace pysme {

void bindReactionParameter(nanobind::module_ &m);

class ReactionParameter {
private:
  ::sme::model::Model *s;
  std::string reacId;
  std::string paramId;

public:
  ReactionParameter() = default;
  explicit ReactionParameter(::sme::model::Model *sbmlDocWrapper,
                             const std::string &reactionId,
                             const std::string &parameterId);
  [[nodiscard]] std::string getName() const;
  void setName(const std::string &name);
  [[nodiscard]] double getValue() const;
  void setValue(double value);
  [[nodiscard]] std::string getStr() const;
};

} // namespace pysme

NB_MAKE_OPAQUE(std::vector<pysme::ReactionParameter>)

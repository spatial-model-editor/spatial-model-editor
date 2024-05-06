#pragma once

#include "sme_reactionparameter.hpp"
#include <nanobind/nanobind.h>
#include <string>
#include <vector>

namespace sme::model {
class Model;
}

namespace pysme {

void bindReaction(nanobind::module_ &m);

class Reaction {
private:
  ::sme::model::Model *s;
  std::string id;

public:
  Reaction() = default;
  explicit Reaction(::sme::model::Model *sbmlDocWrapper,
                    const std::string &sId);
  [[nodiscard]] std::string getName() const;
  void setName(const std::string &name);
  std::vector<ReactionParameter> parameters;
  [[nodiscard]] std::string getStr() const;
};

} // namespace pysme

NB_MAKE_OPAQUE(std::vector<pysme::Reaction>)

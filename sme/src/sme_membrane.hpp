#pragma once

#include "sme_reaction.hpp"
#include "sme_species.hpp"
#include <nanobind/nanobind.h>
#include <string>
#include <vector>

namespace sme::model {
class Model;
}

namespace pysme {

void bindMembrane(nanobind::module_ &m);

class Membrane {
private:
  ::sme::model::Model *s;
  std::string id;

public:
  Membrane() = default;
  explicit Membrane(::sme::model::Model *sbmlDocWrapper,
                    const std::string &sId);
  [[nodiscard]] std::string getName() const;
  void setName(const std::string &name);
  std::vector<Reaction> reactions;
  [[nodiscard]] std::string getStr() const;
};

} // namespace pysme

NB_MAKE_OPAQUE(std::vector<pysme::Membrane>)

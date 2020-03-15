#pragma once

#include <string>

namespace sbml {
class SbmlDocWrapper;
}

namespace pybind11 {
class module;
}

namespace sme {

void pybindReaction(const pybind11::module& m);

class Reaction {
 private:
  sbml::SbmlDocWrapper* s;
  std::string id;

 public:
  explicit Reaction(sbml::SbmlDocWrapper* sbmlDocWrapper,
                    const std::string& speciesId);
  const std::string& getId() const;
  void setName(const std::string& name);
  std::string getName() const;
  std::string getStr() const;
};

}  // namespace sme

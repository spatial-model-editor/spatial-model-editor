#pragma once

#include <pybind11/pybind11.h>
#include <string>

namespace model {
class Model;
}

namespace sme {

void pybindParameter(pybind11::module &m);

class Parameter {
private:
  model::Model *s;
  std::string id;

public:
  Parameter() = default;
  explicit Parameter(model::Model *sbmlDocWrapper, const std::string &sId);
  void setName(const std::string &name);
  std::string getName() const;
  void setValue(const std::string &expr);
  std::string getValue() const;
  std::string getStr() const;
};

} // namespace sme

PYBIND11_MAKE_OPAQUE(std::vector<sme::Parameter>);

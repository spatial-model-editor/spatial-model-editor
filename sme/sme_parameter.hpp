#pragma once

#include <string>

namespace model {
class Model;
}

namespace pybind11 {
class module;
}

namespace sme {

void pybindParameter(const pybind11::module &m);

class Parameter {
private:
  model::Model *s;
  const std::string id;

public:
  explicit Parameter(model::Model *sbmlDocWrapper, std::string sId);
  void setName(const std::string &name);
  std::string getName() const;
  void setValue(const std::string &expr);
  std::string getValue() const;
  std::string getStr() const;
};

} // namespace sme

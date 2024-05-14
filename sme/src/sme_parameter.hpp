#pragma once

#include <nanobind/nanobind.h>
#include <string>
#include <vector>

namespace sme::model {
class Model;
}

namespace pysme {

void bindParameter(nanobind::module_ &m);

class Parameter {
private:
  ::sme::model::Model *s;
  std::string id;

public:
  Parameter() = default;
  explicit Parameter(::sme::model::Model *sbmlDocWrapper,
                     const std::string &sId);
  void setName(const std::string &name);
  [[nodiscard]] std::string getName() const;
  void setValue(const std::string &expr);
  [[nodiscard]] std::string getValue() const;
  [[nodiscard]] std::string getStr() const;
};

} // namespace pysme

NB_MAKE_OPAQUE(std::vector<pysme::Parameter>)

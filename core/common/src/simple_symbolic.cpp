#include "sme/simple_symbolic.hpp"
#include "sme/logger.hpp"
#include <symengine/basic.h>
#include <symengine/parser.h>
#include <symengine/parser/sbml/sbml_parser.h>
#include <symengine/printers/strprinter.h>

namespace sme::common {

using namespace SymEngine;

std::string SimpleSymbolic::divide(const std::string &expr,
                                   const std::string &var) {
  return sbml(*div(parse_sbml(expr), parse_sbml(var)));
}

std::string SimpleSymbolic::multiply(const std::string &expr,
                                     const std::string &var) {
  return sbml(*mul(parse_sbml(expr), parse_sbml(var)));
}

std::string SimpleSymbolic::substitute(
    const std::string &expr,
    const std::vector<std::pair<std::string, double>> &constants) {
  map_basic_basic d;
  for (const auto &[name, value] : constants) {
    SPDLOG_DEBUG("  - constant {} = {}", name, value);
    d[symbol(name)] = real_double(value);
  }
  return sbml(*parse_sbml(expr)->subs(d));
}

bool SimpleSymbolic::contains(const std::string &expr, const std::string &var) {
  return symbols(expr).contains(var);
}

std::set<std::string, std::less<>>
SimpleSymbolic::symbols(const std::string &expr) {
  std::set<std::string, std::less<>> result;
  for (const auto &s : free_symbols(*parse_sbml(expr))) {
    result.insert(rcp_dynamic_cast<const Symbol>(s)->get_name());
  }
  return result;
}

} // namespace sme::common

#include "simple_symbolic.hpp"
#include "logger.hpp"
#include <symengine/basic.h>
#include <symengine/parser.h>
#include <symengine/parser/sbml/sbml_parser.h>
#include <symengine/printers/strprinter.h>

namespace sme::common {

using namespace SymEngine;

static RCP<const Basic> safeParse(const std::string &expr) {
  // hack until https://github.com/symengine/symengine/issues/1566 is resolved:
  // (SymEngine parser relies on strtod and assumes C locale)
  std::locale userLocale{std::locale::global(std::locale::classic())};
  auto e{parse_sbml(expr)};
  std::locale::global(userLocale);
  return e;
}

std::string SimpleSymbolic::divide(const std::string &expr,
                                   const std::string &var) {
  return sbml(*div(safeParse(expr), safeParse(var)));
}

std::string SimpleSymbolic::multiply(const std::string &expr,
                                     const std::string &var) {
  return sbml(*mul(safeParse(expr), safeParse(var)));
}

std::string SimpleSymbolic::substitute(
    const std::string &expr,
    const std::vector<std::pair<std::string, double>> &constants) {
  map_basic_basic d;
  for (const auto &[name, value] : constants) {
    SPDLOG_DEBUG("  - constant {} = {}", name, value);
    d[symbol(name)] = real_double(value);
  }
  return sbml(*safeParse(expr)->subs(d));
}

bool SimpleSymbolic::contains(const std::string &expr, const std::string &var) {
  return symbols(expr).count(var) != 0;
}

std::set<std::string, std::less<>>
SimpleSymbolic::symbols(const std::string &expr) {
  std::set<std::string, std::less<>> result;
  auto fs{free_symbols(*safeParse(expr))};
  for (const auto &s : fs) {
    result.insert(rcp_dynamic_cast<const Symbol>(s)->get_name());
  }
  return result;
}

} // namespace sme::common

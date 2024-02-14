#include "math_test_utils.hpp"
#include <cctype>
#include <symengine/eval_double.h>
#include <symengine/parser.h>
#include <symengine/parser/parser.h>
#include <symengine/visitor.h>

namespace sme::test {

static std::string add_underscores(const std::string &expr) {
  // convert any ".X" to "_X" where X is any non-numeric character to make
  // variables with names like "a.b.c" valid for symengine to parse
  std::string new_expr{expr};
  for (std::size_t i = 0; i + 1 < expr.size(); ++i) {
    if (expr[i] == '.' &&
        !std::isdigit(static_cast<unsigned char>(expr[i + 1]))) {
      new_expr[i] = '_';
    }
  }
  return new_expr;
}

bool symEq(const QString &exprA, const QString &exprB) {
  SymEngine::Parser parser;
  auto a = exprA.split("=");
  auto b = exprB.split("=");
  if (a.size() != b.size()) {
    return false;
  }
  std::cout << "symEq: " << exprA.toStdString() << " == " << exprB.toStdString()
            << std::endl;
  for (int i = 0; i < a.size(); ++i) {
    auto lhs{parser.parse(add_underscores(a[i].toStdString()))};
    auto rhs{parser.parse(add_underscores(b[i].toStdString()))};
    if (lhs->__str__() != rhs->__str__()) {
      auto diff{SymEngine::sub(lhs, rhs)};
      std::cout << *lhs << " != " << *rhs << std::endl;
      std::cout << a[i].toStdString() << " != " << b[i].toStdString()
                << std::endl;
      std::cout << "difference = " << *diff << std::endl;
      // try evaluating diff substituting a different O(1) number for each
      // symbol, if result < 1e-14 treat expressions as equal
      SymEngine::map_basic_basic mapSymbolsToNumbers;
      SymEngine::RCP<const SymEngine::Basic> num(SymEngine::number(1.0145));
      std::cout << "substituting a O(1) number for all symbols:" << std::endl;
      for (const auto &s : SymEngine::free_symbols(*diff)) {
        std::cout << "  " << *s << " <- " << *num << std::endl;
        mapSymbolsToNumbers[s] = num;
        num = SymEngine::add(num, SymEngine::number(0.1645344));
      }
      double numerical_diff =
          std::abs(SymEngine::eval_double(*diff->subs(mapSymbolsToNumbers)));
      std::cout << "numerical difference: " << numerical_diff << std::endl;
      double numerical_sum{0.0};
      for (const auto &expr : {lhs, rhs}) {
        numerical_sum +=
            std::abs(SymEngine::eval_double(*expr->subs(mapSymbolsToNumbers)));
      }
      std::cout << "numerical sum: " << numerical_sum << std::endl;
      double rel_diff{2.0 * numerical_diff / numerical_sum};
      std::cout << "relative difference: " << rel_diff << std::endl;
      if (rel_diff > 1e-13) {
        return false;
      } else {
        std::cout << "is < 1e-13 so assuming expressions are equal"
                  << std::endl;
      }
    }
  }
  return true;
}

bool symEq(const std::string &exprA, const std::string &exprB) {
  QString a{exprA.c_str()};
  QString b{exprB.c_str()};
  return symEq(a, b);
}

} // namespace sme::test

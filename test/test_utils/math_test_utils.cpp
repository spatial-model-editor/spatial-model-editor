#include "math_test_utils.hpp"
#include <symengine/eval_double.h>
#include <symengine/parser.h>
#include <symengine/parser/parser.h>
#include <symengine/visitor.h>

bool symEq(const QString &exprA, const QString &exprB) {
  SymEngine::Parser parser;
  auto a = exprA.split("=");
  auto b = exprB.split("=");
  if (a.size() != b.size()) {
    return false;
  }
  for (int i = 0; i < a.size(); ++i) {
    auto lhs{parser.parse(a[i].toStdString())};
    auto rhs{parser.parse(b[i].toStdString())};
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
      diff = diff->subs(mapSymbolsToNumbers);
      std::cout << "numerical result: " << *diff << std::endl;
      if (std::abs(SymEngine::eval_double(*diff)) > 1e-14) {
        return false;
      } else {
        std::cout << "is < 1e-14 so assuming expressions are equal"
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

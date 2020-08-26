#include "math_test_utils.hpp"
#include <symengine/parser.h>
#include <symengine/parser/parser.h>

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
    std::cout << *lhs << " == " << *rhs << std::endl;
    if (!SymEngine::eq(*lhs, *rhs)) {
      return false;
    }
  }
  return true;
}

bool symEq(const std::string &exprA, const std::string &exprB) {
  QString a{exprA.c_str()};
  QString b{exprB.c_str()};
  return symEq(a, b);
}

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
    if (lhs->__str__() != rhs->__str__()) {
      std::cout << *lhs << " != " << *rhs << std::endl;
      std::cout << a[i].toStdString() << " != " << b[i].toStdString()
                << std::endl;
      std::cout << "difference = " << *(SymEngine::sub(lhs, rhs)) << std::endl;
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

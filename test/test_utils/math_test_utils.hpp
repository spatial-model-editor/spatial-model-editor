#pragma once
#include <QString>
#include <QStringList>
#include <string>

namespace sme::test {

// check if two math expressions are equivalent
bool symEq(const QString &exprA, const QString &exprB);

bool symEq(const std::string &exprA, const std::string &exprB);

} // namespace sme::test

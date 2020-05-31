// SBML math

#pragma once

#include <string>

namespace libsbml {
class Model;
class ASTNode;
} // namespace libsbml

namespace model {

// return supplied math expression as string with any Function calls and/or
// Assignment rules inlined e.g. given mathExpression = "z*f(x,y)" where the
// SBML model contains a function "f(a,b) = a*b-2" it returns "z*(x*y-2)"
std::string inlineFunctions(const std::string &mathExpression,
                            const libsbml::Model *model);

// return supplied math expression as string with any Assignment rules
// inlined
std::string inlineAssignments(const std::string &mathExpression,
                              const libsbml::Model *model);

std::string ASTtoString(const libsbml::ASTNode *node);

} // namespace sbml

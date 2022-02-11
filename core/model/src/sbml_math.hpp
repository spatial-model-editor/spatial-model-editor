// SBML math

#pragma once

#include <map>
#include <memory>
#include <sbml/SBMLTypes.h>
#include <string>
#include <utility>

namespace libsbml {
class Model;
class ASTNode;
} // namespace libsbml

namespace sme::model {

// return supplied math expression as string with any Function calls and/or
// Assignment rules inlined e.g. given mathExpression = "z*f(x,y)" where the
// SBML model contains a function "f(a,b) = a*b-2" it returns "z*(x*y-2)"
// todo: replace with libSBML equivalent
std::string inlineFunctions(const std::string &mathExpression,
                            const libsbml::Model *model);

// return supplied math expression as string with any Assignment rules
// inlined
// todo: replace with libSBML equivalent
std::string inlineAssignments(const std::string &mathExpression,
                              const libsbml::Model *model);

std::string mathASTtoString(const libsbml::ASTNode *node);

std::unique_ptr<libsbml::ASTNode>
mathStringToAST(const std::string &mathExpression,
                const libsbml::Model *model = nullptr);

std::string getUnknownFunctionName(const libsbml::ASTNode *node,
                                   const libsbml::Model *model);

std::string getUnknownVariableName(const libsbml::ASTNode *node,
                                   const libsbml::Model *model);

double evaluateMathAST(
    const libsbml::ASTNode *node,
    const std::map<const std::string, std::pair<double, bool>> &vars = {},
    const libsbml::Model *model = nullptr);

double evaluateMathString(
    const std::string &mathExpression,
    const std::map<const std::string, std::pair<double, bool>> &vars = {},
    const libsbml::Model *model = nullptr);

} // namespace sme::model

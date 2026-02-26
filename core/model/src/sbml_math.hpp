// SBML math

#pragma once

#include "sme/model_functions.hpp"
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

/**
 * @brief Inline model function calls in math expression text.
 */
std::string inlineFunctions(const std::string &mathExpression,
                            const model::ModelFunctions &modelFunctions);

/**
 * @brief Inline assignment rules in math expression text.
 */
std::string inlineAssignments(const std::string &mathExpression,
                              const libsbml::Model *model);

/**
 * @brief Convert AST math node to normalized string.
 */
std::string mathASTtoString(const libsbml::ASTNode *node);

/**
 * @brief Parse math string into libSBML AST.
 */
std::unique_ptr<libsbml::ASTNode>
mathStringToAST(const std::string &mathExpression,
                const libsbml::Model *model = nullptr);

/**
 * @brief Return name of unknown function in AST, if any.
 */
std::string getUnknownFunctionName(const libsbml::ASTNode *node,
                                   const libsbml::Model *model);

/**
 * @brief Return name of unknown variable in AST, if any.
 */
std::string getUnknownVariableName(const libsbml::ASTNode *node,
                                   const libsbml::Model *model);

/**
 * @brief Evaluate AST numerically with optional variable substitutions.
 */
double evaluateMathAST(
    const libsbml::ASTNode *node,
    const std::map<const std::string, std::pair<double, bool>> &vars = {},
    const libsbml::Model *model = nullptr);

/**
 * @brief Parse and evaluate math expression string.
 */
double evaluateMathString(
    const std::string &mathExpression,
    const std::map<const std::string, std::pair<double, bool>> &vars = {},
    const libsbml::Model *model = nullptr);

} // namespace sme::model

#include "sbml_math.hpp"
#include "sme/logger.hpp"
#include "sme/symbolic.hpp"
#include "sme/utils.hpp"
#include <memory>
#include <sbml/SBMLTransforms.h>

namespace sme::model {

std::string inlineFunctions(const std::string &mathExpression,
                            const model::ModelFunctions &modelFunctions) {
  common::Symbolic sym{
      mathExpression, {}, {}, modelFunctions.getSymbolicFunctions(), true};
  return "(" + sym.inlinedExpr() + ")";
}

std::string inlineAssignments(const std::string &mathExpression,
                              const libsbml::Model *model) {
  const std::string delimeters = "()-^*/+, ";
  std::string expr = mathExpression;
  std::string old_expr;
  SPDLOG_DEBUG("inlining {}", expr);
  // iterate through names in expression
  // where names are things in between any of these chars:
  // "()^*/+, "
  // http://sbml.org/Special/Software/libSBML/docs/formatted/cpp-api/class_a_s_t_node.html
  while (expr != old_expr) {
    old_expr = expr;
    auto start = expr.find_first_not_of(delimeters);
    while (start != std::string::npos) {
      auto end = expr.find_first_of(delimeters, start);
      std::string name = expr.substr(start, end - start);
      SPDLOG_TRACE("  - name: {}", name);
      if (const auto *assignment = model->getAssignmentRule(name);
          assignment != nullptr) {
        // replace name with inlined body of Assignment rule
        const std::string &assignmentBody =
            model->getAssignmentRule(name)->getFormula();
        SPDLOG_TRACE("    -> rule: {}", assignmentBody);
        // wrap function body in parentheses
        std::string pre_expr = expr.substr(0, start);
        std::string post_expr;
        if (end != std::string::npos) {
          post_expr = expr.substr(end);
        }
        expr = pre_expr;
        expr.append("(");
        expr.append(assignmentBody);
        expr.append(")");
        expr.append(post_expr);
        SPDLOG_DEBUG("  - new expr = {}", expr);
        // go to end of inlined assignment body in expr
        end = start + assignmentBody.size() + 2;
      }
      start = expr.find_first_not_of(delimeters, end);
    }
  }
  return expr;
}

std::string mathASTtoString(const libsbml::ASTNode *node) {
  if (node == nullptr) {
    return {};
  }
  common::unique_C_ptr<char> charAST{libsbml::SBML_formulaToL3String(node)};
  return charAST.get();
}

std::unique_ptr<libsbml::ASTNode>
mathStringToAST(const std::string &mathExpression,
                const libsbml::Model *model) {
  if (model == nullptr) {
    return std::unique_ptr<libsbml::ASTNode>{
        libsbml::SBML_parseL3Formula(mathExpression.c_str())};
  }
  return std::unique_ptr<libsbml::ASTNode>{
      libsbml::SBML_parseL3FormulaWithModel(mathExpression.c_str(), model)};
}

static const libsbml::ASTNode *
findUnknownName(const libsbml::ASTNode *node, libsbml::ASTNodeType_t nodeType,
                const std::vector<std::string> &names) {
  if (node->getType() == nodeType &&
      std::find(names.cbegin(), names.cend(), node->getName()) ==
          names.cend()) {
    return node;
  }
  for (unsigned int i = 0; i < node->getNumChildren(); ++i) {
    if (const auto *unknown =
            findUnknownName(node->getChild(i), nodeType, names);
        unknown != nullptr) {
      return unknown;
    }
  }
  return nullptr;
}

std::string getUnknownFunctionName(const libsbml::ASTNode *node,
                                   const libsbml::Model *model) {
  std::vector<std::string> functions;
  if (model != nullptr) {
    functions.reserve(
        static_cast<std::size_t>(model->getNumFunctionDefinitions()));
    for (unsigned i = 0; i < model->getNumFunctionDefinitions(); ++i) {
      functions.push_back(model->getFunctionDefinition(i)->getId());
    }
  }
  const auto *unknown = findUnknownName(node, libsbml::AST_FUNCTION, functions);
  if (unknown == nullptr) {
    return {};
  }
  return unknown->getName();
}

std::string getUnknownVariableName(const libsbml::ASTNode *node,
                                   const libsbml::Model *model) {
  std::vector<std::string> ids;
  if (model != nullptr) {
    for (unsigned i = 0; i < model->getNumParameters(); ++i) {
      ids.push_back(model->getParameter(i)->getId());
    }
    for (unsigned i = 0; i < model->getNumSpecies(); ++i) {
      ids.push_back(model->getSpecies(i)->getId());
    }
    for (unsigned i = 0; i < model->getNumCompartments(); ++i) {
      ids.push_back(model->getCompartment(i)->getId());
    }
  }
  const auto *unknown = findUnknownName(node, libsbml::AST_NAME, ids);
  if (unknown == nullptr) {
    return {};
  }
  return unknown->getName();
}

double evaluateMathAST(
    const libsbml::ASTNode *node,
    const std::map<const std::string, std::pair<double, bool>> &vars,
    const libsbml::Model *model) {
  return libsbml::SBMLTransforms::evaluateASTNode(node, vars, model);
}

double evaluateMathString(
    const std::string &mathExpression,
    const std::map<const std::string, std::pair<double, bool>> &vars,
    const libsbml::Model *model) {
  SPDLOG_TRACE("expr: '{}'", mathExpression);
  auto ast = mathStringToAST(mathExpression, model);
  SPDLOG_TRACE("  ->: '{}'", mathASTtoString(ast.get()));
  return libsbml::SBMLTransforms::evaluateASTNode(ast.get(), vars, model);
}

} // namespace sme::model

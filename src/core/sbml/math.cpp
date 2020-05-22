#include "math.hpp"

#include <sbml/SBMLTypes.h>

#include <memory>

#include "logger.hpp"

namespace sbml {

static std::string ASTtoString(const libsbml::ASTNode *node) {
  if (node == nullptr) {
    return {};
  }
  std::unique_ptr<char, decltype(&std::free)> charAST(
      libsbml::SBML_formulaToL3String(node), &std::free);
  return charAST.get();
}

std::string inlineFunctions(
    const std::string &mathExpression, const libsbml::Model* model) {
  std::string expr = mathExpression;
  SPDLOG_DEBUG("inlining {}", expr);
  for (unsigned int i = 0; i < model->getNumFunctionDefinitions(); ++i) {
    const auto *func = model->getFunctionDefinition(i);
    // get copy of function body as AST node
    std::unique_ptr<libsbml::ASTNode> funcBody(func->getBody()->deepCopy());
    // search for function call in expression
    std::string funcCallString = func->getId() + "(";
    auto loc = expr.find(funcCallString);
    auto fn_loc = loc;
    while (loc != std::string::npos) {
      // function call found
      fn_loc = loc;
      loc += func->getId().size() + 1;
      for (unsigned int j = 0; j < func->getNumArguments(); ++j) {
        // compare each argument used in the function call (arg)
        // to the corresponding variable in the function definition
        while (expr[loc] == ' ') {
          // trim any leading spaces
          ++loc;
        }
        auto arg_len = expr.find_first_of(",)", loc + 1) - loc;
        std::string arg = expr.substr(loc, arg_len);
        if (func->getArgument(j)->getName() != arg) {
          // create desired new argument as AST node
          std::unique_ptr<libsbml::ASTNode> argAST(
              libsbml::SBML_parseL3Formula(arg.c_str()));
          // replace existing argument with new argument
          funcBody->replaceArgument(func->getArgument(j)->getName(),
                                    argAST.get());
        }
        loc += arg_len + 1;
      }
      // replace function call with inlined body of function
      std::string funcBodyString = ASTtoString(funcBody.get());
      // wrap function body in parentheses
      std::string pre_expr = expr.substr(0, fn_loc);
      std::string post_expr = expr.substr(loc);
      expr = pre_expr + "(" + funcBodyString + ")" + post_expr;
      // go to end of inlined function body in expr
      loc = fn_loc + funcBodyString.size() + 2;
      SPDLOG_DEBUG("  - new expr = {}", expr);
      // search for next call to same function in expr
      loc = expr.find(funcCallString, loc);
    }
  }
  return expr;
}

std::string inlineAssignments(
    const std::string &mathExpression, const libsbml::Model* model) {
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
        expr = pre_expr + "(" + assignmentBody + ")" + post_expr;
        SPDLOG_DEBUG("  - new expr = {}", expr);
        // go to end of inlined assignment body in expr
        end = start + assignmentBody.size() + 2;
      }
      start = expr.find_first_not_of(delimeters, end);
    }
  }
  return expr;
}

}  // namespace sbml

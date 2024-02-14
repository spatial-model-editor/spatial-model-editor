#include "sme/model_math.hpp"
#include "sbml_math.hpp"
#include "sme/utils.hpp"
#include <memory>
#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>

namespace sme::model {

ModelMath::ModelMath() = default;

ModelMath::ModelMath(const libsbml::Model *model) : sbmlModel{model} {}

void ModelMath::parse(const std::string &expr) {
  if (expr.empty()) {
    valid = false;
    errorMessage = "Empty expression";
    return;
  }
  astNode = mathStringToAST(expr, sbmlModel);
  if (astNode == nullptr) {
    valid = false;
    common::unique_C_ptr<char> err{libsbml::SBML_getLastParseL3Error()};
    errorMessage = err.get();
    return;
  }
  if (auto unknownFunc = getUnknownFunctionName(astNode.get(), sbmlModel);
      !unknownFunc.empty()) {
    valid = false;
    errorMessage = "Unknown function: " + unknownFunc;
    return;
  }
  if (auto unknownVar = getUnknownVariableName(astNode.get(), sbmlModel);
      !unknownVar.empty()) {
    valid = false;
    errorMessage = "Unknown variable: " + unknownVar;
    return;
  }
  valid = true;
  errorMessage.clear();
}

double ModelMath::eval(
    const std::map<const std::string, std::pair<double, bool>> &vars) const {
  return evaluateMathAST(astNode.get(), vars, sbmlModel);
}

bool ModelMath::isValid() const { return valid; }

const std::string &ModelMath::getErrorMessage() const { return errorMessage; }

} // namespace sme::model

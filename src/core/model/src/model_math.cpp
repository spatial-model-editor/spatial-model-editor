#include "model_math.hpp"

#include "math.hpp"
#include <memory>
#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>

namespace model {

ModelMath::ModelMath() = default;

ModelMath::ModelMath(const libsbml::Model *model) : sbmlModel{model} {};

void ModelMath::parse(const std::string &expr) {
  if (expr.empty()) {
    valid = false;
    errorMessage = "Empty expression";
    return;
  }
  astNode = mathStringToAST(expr, sbmlModel);
  if (astNode == nullptr) {
    valid = false;
    std::unique_ptr<char, decltype(&std::free)> err(
        libsbml::SBML_getLastParseL3Error(), &std::free);
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

ModelMath::ModelMath(ModelMath &&) noexcept = default;

ModelMath &ModelMath::operator=(ModelMath &&) noexcept = default;

ModelMath::~ModelMath() = default;

}; // namespace model

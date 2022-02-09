#include "sme/model_math.hpp"
#include "sbml_math.hpp"
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

// note: these move constructors could just be defaulted,
// but are explicitly written out here to avoid an issue with noexcept default
// constructors with gcc8
ModelMath::ModelMath(ModelMath &&that) noexcept
    : sbmlModel{that.sbmlModel}, astNode{std::move(that.astNode)},
      valid{that.valid}, errorMessage{std::move(that.errorMessage)} {}

ModelMath &ModelMath::operator=(ModelMath &&that) noexcept {
  sbmlModel = std::move(that.sbmlModel);
  astNode = std::move(that.astNode);
  valid = std::move(that.valid);
  errorMessage = std::move(that.errorMessage);
  return *this;
}

ModelMath::~ModelMath() = default;

} // namespace sme::model

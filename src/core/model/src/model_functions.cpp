#include "model_functions.hpp"

#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>

#include <QString>
#include <memory>

#include "id.hpp"
#include "logger.hpp"
#include "math.hpp"

namespace model {

static QStringList importIds(const libsbml::Model *model) {
  QStringList ids;
  unsigned int numFunctions = model->getNumFunctionDefinitions();
  ids.reserve(static_cast<int>(numFunctions));
  for (unsigned int i = 0; i < numFunctions; ++i) {
    const auto *func = model->getFunctionDefinition(i);
    ids.push_back(func->getId().c_str());
  }
  return ids;
}

static QStringList importNamesAndMakeUnique(libsbml::Model *model) {
  QStringList names;
  unsigned int numFunctions = model->getNumFunctionDefinitions();
  names.reserve(static_cast<int>(numFunctions));
  for (unsigned int i = 0; i < numFunctions; ++i) {
    auto *func = model->getFunctionDefinition(i);
    auto sId = func->getId();
    if (func->getName().empty()) {
      SPDLOG_INFO("FunctionDefinition '{0}' has no Name, using '{0}'", sId);
      func->setName(sId);
    }
    std::string name = func->getName();
    while (names.contains(name.c_str())) {
      name.append("_");
      func->setName(name);
      SPDLOG_INFO(
          "Changing FunctionDefinition '{}' name to '{}' to make it unique",
          sId, name);
    }
    names.push_back(QString::fromStdString(name));
  }
  return names;
}

ModelFunctions::ModelFunctions() = default;

ModelFunctions::ModelFunctions(libsbml::Model *model)
    : ids{importIds(model)},
      names{importNamesAndMakeUnique(model)},
      sbmlModel{model} {}

const QStringList &ModelFunctions::getIds() const { return ids; }

QString ModelFunctions::getName(const QString &id) const {
  const auto *f = sbmlModel->getFunctionDefinition(id.toStdString());
  return f->getName().c_str();
}

Func ModelFunctions::getDefinition(const QString &id) const {
  Func f;
  const auto *func = sbmlModel->getFunctionDefinition(id.toStdString());
  if (func == nullptr) {
    SPDLOG_WARN("function {} does not exist", id.toStdString());
    return {};
  }
  f.id = func->getId();
  f.name = func->getName();
  f.expression = ASTtoString(func->getBody());
  f.arguments.reserve(func->getNumArguments());
  for (unsigned i = 0; i < func->getNumArguments(); ++i) {
    f.arguments.push_back(ASTtoString(func->getArgument(i)));
  }
  return f;
}

static libsbml::ASTNode *newLambdaBvar(const std::string &variableName) {
  libsbml::ASTNode *n = new libsbml::ASTNode(libsbml::ASTNodeType_t::AST_NAME);
  n->setBvar();
  n->setName(variableName.c_str());
  return n;
}

void ModelFunctions::setDefinition(const Func &func) {
  SPDLOG_INFO("Setting function");
  SPDLOG_INFO("  - Id: {}", func.id);
  auto *f = sbmlModel->getFunctionDefinition(func.id);
  SPDLOG_INFO("  - Name: {}", func.name);
  f->setName(func.name);
  auto i = ids.indexOf(func.id.c_str());
  names[i] = func.name.c_str();
  auto lambdaAST =
      std::make_unique<libsbml::ASTNode>(libsbml::ASTNodeType_t::AST_LAMBDA);
  for (const auto &arg : func.arguments) {
    SPDLOG_INFO("  - arg: {}", arg);
    lambdaAST->addChild(newLambdaBvar(arg));
  }
  SPDLOG_INFO("  - expr: {}", func.expression);
  auto bodyAST = libsbml::SBML_parseL3Formula(func.expression.c_str());
  if (bodyAST == nullptr) {
    SPDLOG_ERROR("  - libSBML failed to parse expression");
    return;
  }
  lambdaAST->addChild(bodyAST);
  SPDLOG_DEBUG("  - ast: {}", ASTtoString(lambdaAST.get()));
  if (!lambdaAST->isWellFormedASTNode()) {
    SPDLOG_ERROR("  - AST node is not well formed");
    return;
  }
  f->setMath(lambdaAST.get());
}

void ModelFunctions::add(const QString &functionName) {
  auto functionId = nameToUniqueSId(functionName, sbmlModel).toStdString();
  SPDLOG_INFO("Adding function");
  SPDLOG_INFO("  - Id: {}", functionId);
  SPDLOG_INFO("  - Name: {}", functionName.toStdString());
  auto *func = sbmlModel->createFunctionDefinition();
  auto lambdaAST =
      std::make_unique<libsbml::ASTNode>(libsbml::ASTNodeType_t::AST_LAMBDA);
  lambdaAST->addChild(libsbml::SBML_parseL3Formula("0"));
  SPDLOG_DEBUG("  - AST: {}", ASTtoString(lambdaAST.get()));
  func->setId(functionId);
  func->setName(functionName.toStdString());
  func->setMath(lambdaAST.get());
  ids.push_back(functionId.c_str());
  names.push_back(functionName);
}

void ModelFunctions::remove(const QString &id) {
  std::string sId{id.toStdString()};
  SPDLOG_INFO("Removing function {}", sId);
  std::unique_ptr<libsbml::FunctionDefinition> rmfunc(
      sbmlModel->removeFunctionDefinition(sId));
  if (rmfunc == nullptr) {
    SPDLOG_WARN("  - function {} not found", sId);
    return;
  }
  SPDLOG_INFO("  - function {} removed", rmfunc->getId());
  auto i = ids.indexOf(id);
  ids.removeAt(i);
  names.removeAt(i);
}

}  // namespace model

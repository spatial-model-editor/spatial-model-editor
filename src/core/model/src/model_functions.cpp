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
    : ids{importIds(model)}, names{importNamesAndMakeUnique(model)},
      sbmlModel{model} {}

const QStringList &ModelFunctions::getIds() const { return ids; }

QString ModelFunctions::setName(const QString &id, const QString &name) {
  auto i = ids.indexOf(id);
  if (i < 0) {
    return {};
  }
  if (names[i] == name) {
    // no-op: setting name to the same value as it already had
    return name;
  }
  auto uniqueName = makeUnique(name, names);
  names[i] = uniqueName;
  std::string sId{id.toStdString()};
  std::string sName{uniqueName.toStdString()};
  auto *reac = sbmlModel->getFunctionDefinition(sId);
  SPDLOG_INFO("sId '{}' : name -> '{}'", sId, sName);
  reac->setName(sName);
  return uniqueName;
}

QString ModelFunctions::getName(const QString &id) const {
  auto i = ids.indexOf(id);
  if (i < 0) {
    return {};
  }
  return names[i];
}

void ModelFunctions::setExpression(const QString &id,
                                   const QString &expression) {
  auto lambdaAST =
      std::make_unique<libsbml::ASTNode>(libsbml::ASTNodeType_t::AST_LAMBDA);
  auto *func = sbmlModel->getFunctionDefinition(id.toStdString());
  for (unsigned i = 0; i < func->getNumArguments(); ++i) {
    lambdaAST->addChild(func->getMath()->getChild(i)->deepCopy());
  }
  std::string expr = expression.toStdString();
  auto bodyAST = libsbml::SBML_parseL3FormulaWithModel(expr.c_str(), sbmlModel);
  if (bodyAST == nullptr) {
    SPDLOG_ERROR("  - libSBML failed to parse expression");
    return;
  }
  lambdaAST->addChild(bodyAST);
  if (!lambdaAST->isWellFormedASTNode()) {
    SPDLOG_ERROR("  - AST node is not well formed");
    return;
  }
  func->setMath(lambdaAST.get());
}

QString ModelFunctions::getExpression(const QString &id) const {
  const auto *func = sbmlModel->getFunctionDefinition(id.toStdString());
  return mathASTtoString(func->getBody()).c_str();
}

static libsbml::ASTNode *newLambdaBvar(const std::string &variableName) {
  libsbml::ASTNode *n = new libsbml::ASTNode(libsbml::ASTNodeType_t::AST_NAME);
  n->setBvar();
  n->setName(variableName.c_str());
  return n;
}

void ModelFunctions::addArgument(const QString &functionId,
                                 const QString &argumentId) {
  std::string argId = argumentId.toStdString();
  auto lambdaAST =
      std::make_unique<libsbml::ASTNode>(libsbml::ASTNodeType_t::AST_LAMBDA);
  auto *func = sbmlModel->getFunctionDefinition(functionId.toStdString());
  for (unsigned i = 0; i < func->getNumArguments(); ++i) {
    const auto *child = func->getMath()->getChild(i);
    SPDLOG_TRACE("  + {}", child->getName());
    lambdaAST->addChild(child->deepCopy());
  }
  lambdaAST->addChild(newLambdaBvar(argId));
  SPDLOG_TRACE("  + {}", argId);
  lambdaAST->addChild(func->getBody()->deepCopy());
  func->setMath(lambdaAST.get());
}

void ModelFunctions::removeArgument(const QString &functionId,
                                    const QString &argumentId) {
  std::string argName = argumentId.toStdString();
  SPDLOG_TRACE("Removing argument '{}' from function '{}'", argName,
               functionId.toStdString());
  auto lambdaAST =
      std::make_unique<libsbml::ASTNode>(libsbml::ASTNodeType_t::AST_LAMBDA);
  auto *func = sbmlModel->getFunctionDefinition(functionId.toStdString());
  for (unsigned i = 0; i < func->getNumArguments(); ++i) {
    if (const auto *child = func->getMath()->getChild(i);
        child->getName() != argName) {
      SPDLOG_TRACE("  + {}", child->getName());
      lambdaAST->addChild(child->deepCopy());
    }
  }
  lambdaAST->addChild(func->getBody()->deepCopy());
  func->setMath(lambdaAST.get());
}

QStringList ModelFunctions::getArguments(const QString &id) const {
  QStringList args;
  const auto *func = sbmlModel->getFunctionDefinition(id.toStdString());
  args.reserve(static_cast<int>(func->getNumArguments()));
  for (unsigned i = 0; i < func->getNumArguments(); ++i) {
    args.push_back(func->getMath()->getChild(i)->getName());
  }
  return args;
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
  SPDLOG_DEBUG("  - AST: {}", mathASTtoString(lambdaAST.get()));
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

} // namespace model

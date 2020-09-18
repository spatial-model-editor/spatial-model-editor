#include "model_functions.hpp"

#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>

#include <QString>
#include <memory>

#include "id.hpp"
#include "logger.hpp"
#include "sbml_math.hpp"

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

const QStringList &ModelFunctions::getNames() const { return names; }

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
  auto *func = sbmlModel->getFunctionDefinition(sId);
  SPDLOG_INFO("sId '{}' : name -> '{}'", sId, sName);
  func->setName(sName);
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
  SPDLOG_INFO("{}", expr);
  auto *bodyAST =
      libsbml::SBML_parseL3FormulaWithModel(expr.c_str(), sbmlModel);
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

QStringList ModelFunctions::getArguments(const QString &id) const {
  QStringList args;
  const auto *func = sbmlModel->getFunctionDefinition(id.toStdString());
  args.reserve(static_cast<int>(func->getNumArguments()));
  for (unsigned i = 0; i < func->getNumArguments(); ++i) {
    args.push_back(func->getMath()->getChild(i)->getName());
  }
  return args;
}

static libsbml::ASTNode *newLambdaBvar(const std::string &name) {
  libsbml::ASTNode *n = new libsbml::ASTNode(libsbml::ASTNodeType_t::AST_NAME);
  n->setBvar();
  n->setName(name.c_str());
  return n;
}

static std::string
makeValidArgumentName(const std::string &name,
                      const libsbml::FunctionDefinition *func) {
  std::string s;
  // first char must be a letter or underscore
  if (auto c = name.front();
      !(std::isalpha(c, std::locale::classic()) || c == '_')) {
    s.append("_");
  }
  // other chars must be letters, numbers or underscores
  for (const auto c : name) {
    if (std::isalnum(c, std::locale::classic()) || c == '_') {
      s += c;
    }
  }
  // ensure argument name is unique within this function definition
  while (func->getArgument(s) != nullptr) {
    s += "_";
  }
  return s;
}

QString ModelFunctions::addArgument(const QString &functionId,
                                    const QString &argumentId) {
  auto *func = sbmlModel->getFunctionDefinition(functionId.toStdString());
  std::string argId{makeValidArgumentName(argumentId.toStdString(), func)};
  auto lambdaAST =
      std::make_unique<libsbml::ASTNode>(libsbml::ASTNodeType_t::AST_LAMBDA);
  for (unsigned i = 0; i < func->getNumArguments(); ++i) {
    const auto *child = func->getMath()->getChild(i);
    SPDLOG_TRACE("  + {}", child->getName());
    lambdaAST->addChild(child->deepCopy());
  }
  lambdaAST->addChild(newLambdaBvar(argId));
  SPDLOG_TRACE("  + {}", argId);
  lambdaAST->addChild(func->getBody()->deepCopy());
  func->setMath(lambdaAST.get());
  return argId.c_str();
}

void ModelFunctions::removeArgument(const QString &functionId,
                                    const QString &argumentId) {
  std::string argId = argumentId.toStdString();
  SPDLOG_TRACE("Removing argument '{}' from function '{}'", argId,
               functionId.toStdString());
  auto lambdaAST =
      std::make_unique<libsbml::ASTNode>(libsbml::ASTNodeType_t::AST_LAMBDA);
  auto *func = sbmlModel->getFunctionDefinition(functionId.toStdString());
  for (unsigned i = 0; i < func->getNumArguments(); ++i) {
    if (const auto *child = func->getMath()->getChild(i);
        child->getName() != argId) {
      SPDLOG_TRACE("  + {}", child->getName());
      lambdaAST->addChild(child->deepCopy());
    }
  }
  lambdaAST->addChild(func->getBody()->deepCopy());
  func->setMath(lambdaAST.get());
}

QString ModelFunctions::add(const QString &name) {
  auto uniqueName = makeUnique(name, names);
  auto id = nameToUniqueSId(uniqueName, sbmlModel).toStdString();
  SPDLOG_INFO("Adding function");
  SPDLOG_INFO("  - Id: {}", id);
  SPDLOG_INFO("  - Name: {}", uniqueName.toStdString());
  auto *func = sbmlModel->createFunctionDefinition();
  auto lambdaAST =
      std::make_unique<libsbml::ASTNode>(libsbml::ASTNodeType_t::AST_LAMBDA);
  lambdaAST->addChild(libsbml::SBML_parseL3Formula("0"));
  SPDLOG_DEBUG("  - AST: {}", mathASTtoString(lambdaAST.get()));
  func->setId(id);
  func->setName(uniqueName.toStdString());
  func->setMath(lambdaAST.get());
  ids.push_back(id.c_str());
  names.push_back(uniqueName);
  return uniqueName;
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

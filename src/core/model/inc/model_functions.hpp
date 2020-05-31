// SBML functions

#pragma once

#include <QColor>
#include <QStringList>
#include <map>
#include <optional>
#include <string>

namespace libsbml {
class Model;
}

namespace model {

struct Func {
  std::string id;
  std::string name;
  std::string expression;
  std::vector<std::string> arguments;
};

class ModelFunctions {
private:
  QStringList ids;
  QStringList names;
  libsbml::Model *sbmlModel = nullptr;

public:
  ModelFunctions();
  explicit ModelFunctions(libsbml::Model *model);
  const QStringList &getIds() const;
  QString getName(const QString &id) const;
  Func getDefinition(const QString &id) const;
  void setDefinition(const Func &func);
  void add(const QString &functionName);
  void remove(const QString &functionID);
};

} // namespace sbml

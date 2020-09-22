#include "qplaintextmathedit.hpp"

#include <QString>
#include <algorithm>
#include <limits>
#include <locale>

#include "logger.hpp"
#include "model_math.hpp"

void QPlainTextMathEdit::enableLibSbmlBackend(model::ModelMath *math) {
  modelMath = math;
  useLibSbmlBackend = true;
  allowImplicitNames = true;
  allowIllegalChars = true;
}

bool QPlainTextMathEdit::mathIsValid() const { return expressionIsValid; }

const QString &QPlainTextMathEdit::getMath() const {
  return currentDisplayMath;
}

const std::string &QPlainTextMathEdit::getVariableMath() const {
  return currentVariableMath;
}

void QPlainTextMathEdit::importVariableMath(const std::string &expr) {
  setPlainText(variablesToDisplayNames(expr).c_str());
}

void QPlainTextMathEdit::compileMath() {
  if (useLibSbmlBackend) {
    return;
  }
  sym.compile();
}

double QPlainTextMathEdit::evaluateMath(const std::vector<double> &variables) {
  if (useLibSbmlBackend) {
    if (!variables.empty()) {
      SPDLOG_ERROR(
          "Wrong interace: use map interface when using libSBML backend");
      return std::numeric_limits<double>::quiet_NaN();
    }
    return modelMath->eval();
  }
  sym.eval(result, variables);
  return result[0];
}

double QPlainTextMathEdit::evaluateMath(
    const std::map<const std::string, std::pair<double, bool>> &variables) {
  if (!useLibSbmlBackend) {
    SPDLOG_WARN("for better performance use vector interface when not using "
                "libSBML backend");
    std::vector<double> values(vars.size(), 0);
    for (std::size_t i = 0; i < vars.size(); ++i) {
      values[i] = variables.at(vars[i]).first;
    }
    return evaluateMath(values);
  }
  return modelMath->eval(variables);
}

const QString &QPlainTextMathEdit::getErrorMessage() const {
  return currentErrorMessage;
}

const std::vector<std::string> &QPlainTextMathEdit::getVariables() const {
  return vars;
}

void QPlainTextMathEdit::clearVariables() {
  vars.clear();
  mapDisplayNamesToVars.clear();
  mapVarsToDisplayNames.clear();
  qPlainTextEdit_textChanged();
}

void QPlainTextMathEdit::setVariables(
    const std::vector<std::string> &variables) {
  clearVariables();
  for (const auto &v : variables) {
    addVariable(v, v);
  }
  qPlainTextEdit_textChanged();
}

static bool isValidSymbol(const std::string &name) {
  // first char must be a letter or underscore
  if (auto c = name.front();
      !(std::isalpha(c, std::locale::classic()) || c == '_')) {
    return false;
  }
  // other chars must be letters, numbers or underscores
  for (auto c : name) {
    if (!(std::isalnum(c, std::locale::classic()) || c == '_')) {
      return false;
    }
  }
  return true;
}

void QPlainTextMathEdit::addVariable(const std::string &variable,
                                     const std::string &displayName) {
  SPDLOG_TRACE("adding var: {}", variable);
  vars.push_back(variable);
  auto name = displayName;
  if (displayName.empty()) {
    name = variable;
  }
  SPDLOG_TRACE("  -> display name {}", name);
  mapDisplayNamesToVars[name] = variable;
  if (isValidSymbol(name)) {
    mapVarsToDisplayNames[variable] = name;
  } else {
    mapVarsToDisplayNames[variable] = "\"" + name + "\"";
  }
  qPlainTextEdit_textChanged();
}

void QPlainTextMathEdit::removeVariable(const std::string &variable) {
  vars.erase(std::remove(vars.begin(), vars.end(), variable), vars.end());
  if (auto iter = mapVarsToDisplayNames.find(variable);
      iter != mapVarsToDisplayNames.cend()) {
    SPDLOG_TRACE("removing variable: {}", variable);
    std::string displayName = iter->second;
    // remove quotes if present
    if (displayName.front() == '"' && displayName.back() == '"') {
      displayName = displayName.substr(1, displayName.size() - 2);
    }
    SPDLOG_TRACE("  -> display name {}", displayName);
    mapDisplayNamesToVars.erase(displayName);
    mapVarsToDisplayNames.erase(iter);
  }
  qPlainTextEdit_textChanged();
}

void QPlainTextMathEdit::clearFunctions() {
  mapFuncsToDisplayNames.clear();
  mapDisplayNamesToFuncs.clear();
  functions.clear();
  qPlainTextEdit_textChanged();
}

void QPlainTextMathEdit::resetToDefaultFunctions() {
  clearFunctions();
  for (const auto &f :
       {"sin", "cos", "tan", "exp", "log", "ln", "pow", "sqrt"}) {
    addIntrinsicFunction(f);
  }
  qPlainTextEdit_textChanged();
}

void QPlainTextMathEdit::addIntrinsicFunction(const std::string &functionId) {
  SPDLOG_TRACE("adding intrinsic function: {}", functionId);
  if (!isValidSymbol(functionId)) {
    SPDLOG_WARN("invalid intrinsic function id: '{}' - ignoring", functionId);
    return;
  }
  mapDisplayNamesToFuncs[functionId] = functionId;
  mapFuncsToDisplayNames[functionId] = functionId;
}

void QPlainTextMathEdit::addFunction(const symbolic::Function &function) {
  SPDLOG_TRACE("adding function: {}", function.id);
  functions.push_back(function);
  SPDLOG_TRACE("  -> display name {}", function.name);
  mapDisplayNamesToFuncs[function.name] = function.id;
  if (isValidSymbol(function.name)) {
    mapFuncsToDisplayNames[function.id] = function.name;
  } else {
    mapFuncsToDisplayNames[function.id] = "\"" + function.name + "\"";
  }
  qPlainTextEdit_textChanged();
}

void QPlainTextMathEdit::removeFunction(const std::string &functionId) {
  if (auto iter = mapFuncsToDisplayNames.find(functionId);
      iter != mapFuncsToDisplayNames.cend()) {
    SPDLOG_TRACE("removing function: {}", functionId);
    std::string displayName = iter->second;
    // remove quotes if present
    if (displayName.front() == '"' && displayName.back() == '"') {
      displayName = displayName.substr(1, displayName.size() - 2);
    }
    SPDLOG_TRACE("  -> display name {}", displayName);
    mapDisplayNamesToFuncs.erase(displayName);
    mapFuncsToDisplayNames.erase(iter);
  }
  functions.erase(std::remove_if(functions.begin(), functions.end(),
                                 [&functionId](const auto &f) {
                                   return f.id == functionId;
                                 }),
                  functions.end());
  qPlainTextEdit_textChanged();
}

QPlainTextMathEdit::QPlainTextMathEdit(QWidget *parent)
    : QPlainTextEdit(parent) {
  resetToDefaultFunctions();
  connect(this, &QPlainTextEdit::textChanged, this,
          &QPlainTextMathEdit::qPlainTextEdit_textChanged);
  connect(this, &QPlainTextEdit::cursorPositionChanged, this,
          &QPlainTextMathEdit::qPlainTextEdit_cursorPositionChanged);
}

// - iterate through each symbol in expr
// - a symbol is any text with a delimeter char before and after it
// - a function is a symbol that is directly followed by "("
// - note: special case of scientific notation number e.g. 1e-3
// - look-up symbol in map and replace it with result
// - if not found in map and allowImplicitNames=false, return error message
static std::pair<std::string, QString> substitute(
    const std::string &expr, const std::map<std::string, std::string> &varMap,
    const std::map<std::string, std::string> &funcMap, bool allowImplicitNames,
    const std::string &delimeters = "()-^*/+, ") {
  SPDLOG_DEBUG("expr: {}", expr);
  if (expr.empty()) {
    return {};
  }
  std::string out;
  out.reserve(expr.size());
  // skip over any initial delimeters
  auto start = expr.find_first_not_of(delimeters);
  // append them to output
  out.append(expr.substr(0, start));
  while (start != std::string::npos) {
    // find next delimeter
    auto end = expr.find_first_of(delimeters, start);
    if (std::isdigit(expr[start], std::locale::classic()) &&
        end < expr.size() && expr[end - 1] == 'e' &&
        (expr[end] == '-' || expr[end] == '+')) {
      // if symbol starts with a numerical digit, and ends with "e-" or "e+",
      // we have the first half of a number in scientific notation, so carry
      // on to next delimeter to get the rest of the number
      end = expr.find_first_of(delimeters, end + 1);
    }
    std::string var = expr.substr(start, end - start);
    SPDLOG_DEBUG("  - var {}", var);
    if (end < expr.size() && expr[end] == '(') {
      // function
      if (auto iter = funcMap.find(var); iter != funcMap.cend()) {
        SPDLOG_DEBUG("    -> {} ", iter->second);
        out.append(iter->second);
      } else if (allowImplicitNames) {
        out.append(var);
      } else {
        SPDLOG_DEBUG("    -> function not found");
        return {out, QString("function '%1' not found").arg(var.c_str())};
      }
    } else {
      // variable
      if (auto iter = varMap.find(var); iter != varMap.cend()) {
        // replace variable with map result
        SPDLOG_DEBUG("    -> {} ", iter->second);
        out.append(iter->second);
      } else {
        // if not found, check if it is a number, if so append it and continue
        bool isValidDouble{false};
        QString(var.c_str()).toDouble(&isValidDouble);
        if (allowImplicitNames || isValidDouble) {
          out.append(var);
        } else {
          SPDLOG_DEBUG("    -> variable not found");
          return {out, QString("variable '%1' not found").arg(var.c_str())};
        }
      }
    }
    if (end == std::string::npos) {
      break;
    }
    // skip over any delimeters to start of next variable
    start = expr.find_first_not_of(delimeters, end);
    // append them to output
    out.append(expr.substr(end, start - end));
  }
  SPDLOG_DEBUG("  -> {}", out);
  return {out, {}};
}

std::string
QPlainTextMathEdit::variablesToDisplayNames(const std::string &expr) const {
  const auto &varMap = mapVarsToDisplayNames;
  const auto &funcMap = mapFuncsToDisplayNames;
  if (varMap.empty() && funcMap.empty()) {
    return expr;
  }
  return substitute(expr, varMap, funcMap, allowImplicitNames).first;
}

std::pair<std::string, QString>
QPlainTextMathEdit::displayNamesToVariables(const std::string &expr) const {
  if (expr.empty()) {
    return {};
  }
  std::string out;
  const char quoteChar = '"';
  const auto &varMap = mapDisplayNamesToVars;
  const auto &funcMap = mapDisplayNamesToFuncs;
  if (varMap.empty()) {
    return {expr, ""};
  }
  // find first quote char
  auto start = expr.find(quoteChar);
  if (start > 0) {
    // substitute any text before opening quote
    auto subs =
        substitute(expr.substr(0, start), varMap, funcMap, allowImplicitNames);
    if (subs.second.isEmpty()) {
      out.append(subs.first);
    } else {
      return subs;
    }
  }
  while (start != std::string::npos) {
    // skip over opening quote char
    ++start;
    // find closing quote char
    auto end = expr.find(quoteChar, start);
    if (end == std::string::npos) {
      SPDLOG_DEBUG("  - no closing quote");
      return {out, "no closing quote"};
    }
    // extract name & replace with variable or function if found
    std::string name = expr.substr(start, end - start);
    SPDLOG_TRACE("  - name {} (chars {}-{})", name, start, end);
    if (auto varIter = varMap.find(name); varIter != varMap.cend()) {
      out.append(varIter->second);
    } else if (auto funcIter = funcMap.find(name); funcIter != funcMap.cend()) {
      out.append(funcIter->second);
    } else if (allowImplicitNames) {
      out.append(name);
    } else {
      SPDLOG_DEBUG("  - name {} not found", name);
      return {out, QString("name '%1' not found").arg(name.c_str())};
    }
    SPDLOG_TRACE("  - out: {}", out);
    // skip over closing quote char
    ++end;
    // find next opening quote char
    start = expr.find(quoteChar, end);
    if (start > end) {
      // substitute any text before next "
      auto subs = substitute(expr.substr(end, start - end), varMap, funcMap,
                             allowImplicitNames);
      if (subs.second.isEmpty()) {
        out.append(subs.first);
      } else {
        return subs;
      }
    }
  }
  SPDLOG_DEBUG(" -> {}", out);
  return {out, {}};
}

void QPlainTextMathEdit::qPlainTextEdit_textChanged() {
  expressionIsValid = false;
  currentDisplayMath.clear();
  currentVariableMath.clear();
  currentErrorMessage.clear();
  // convert display names in expression to variables
  auto [newExpr, errorMessage] =
      displayNamesToVariables(toPlainText().toStdString());
  if (!errorMessage.isEmpty()) {
    currentErrorMessage = errorMessage;
  }
  if (!allowIllegalChars) {
    // check for illegal chars
    if (auto i = newExpr.find_first_of(illegalChars); i != std::string::npos) {
      currentErrorMessage = QString("Illegal character: %1").arg(newExpr[i]);
    }
  }
  if (newExpr.empty() && currentErrorMessage.isEmpty()) {
    currentErrorMessage = QString("Empty expression");
  }
  if (currentErrorMessage.isEmpty()) {
    // parse (but don't compile) symbolic expression
    if (useLibSbmlBackend) {
      SPDLOG_DEBUG("parsing '{}' with libSBML backend", newExpr);
      modelMath->parse(newExpr);
      if (modelMath->isValid()) {
        expressionIsValid = true;
        currentErrorMessage = "";
        currentVariableMath = newExpr;
        currentDisplayMath =
            variablesToDisplayNames(currentVariableMath).c_str();
      } else {
        // if SymEngine failed to parse, capture error message
        SPDLOG_DEBUG("  -> {}", modelMath->getErrorMessage().c_str());
        currentErrorMessage = modelMath->getErrorMessage().c_str();
      }
    } else {
      SPDLOG_DEBUG("parsing '{}' with SymEngine backend", newExpr);
      sym = symbolic::Symbolic(newExpr, vars, {}, functions, false);
      if (sym.isValid()) {
        expressionIsValid = true;
        currentErrorMessage = "";
        currentVariableMath = sym.expr();
        currentDisplayMath =
            variablesToDisplayNames(currentVariableMath).c_str();
      } else {
        // if SymEngine failed to parse, capture error message
        currentErrorMessage = sym.getErrorMessage().c_str();
      }
    }
  }
  emit mathChanged(currentDisplayMath, expressionIsValid, currentErrorMessage);
}

static std::pair<int, bool> getClosingBracket(const QString &expr, int pos,
                                              int sign) {
  int len = 0;
  int count = sign;
  int iEnd = (sign < 0) ? -1 : expr.size();
  for (int i = pos + sign; i != iEnd; i += sign) {
    ++len;
    if (expr[i] == ')') {
      --count;
    } else if (expr[i] == '(') {
      ++count;
    }
    if (count == 0) {
      // found matching bracket
      return {len + 1, true};
    }
  }
  // did not find closing bracket
  return {len + 1, false};
}

void QPlainTextMathEdit::qPlainTextEdit_cursorPositionChanged() {
  // very basic syntax highlighting:
  // if cursor is before a '(' or after a ')'
  // highlight text between matching braces in green
  // if there is no matching brace, highlight with red
  auto expr = toPlainText();
  QTextEdit::ExtraSelection s{textCursor(), {}};
  int i = s.cursor.position();
  if (i < expr.size() && expr[i] == '(') {
    auto [len, valid] = getClosingBracket(expr, i, +1);
    const auto &col = valid ? colourValid : colourInvalid;
    s.cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor,
                          len);
    s.format.setBackground(QBrush(col));
  } else if (i > 0 && expr[i - 1] == ')') {
    auto [len, valid] = getClosingBracket(expr, i - 1, -1);
    const auto &col = valid ? colourValid : colourInvalid;
    s.cursor.movePosition(QTextCursor::PreviousCharacter,
                          QTextCursor::KeepAnchor, len);
    s.format.setBackground(QBrush(col));
  }
  setExtraSelections({s});
}

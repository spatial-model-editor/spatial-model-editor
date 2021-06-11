#include "qplaintextmathedit.hpp"
#include "logger.hpp"
#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QScrollBar>
#include <QString>
#include <algorithm>
#include <limits>
#include <locale>

static constexpr char quoteChar{'"'};

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

void QPlainTextMathEdit::compileMath() { sym.compile(); }

double QPlainTextMathEdit::evaluateMath(const std::vector<double> &variables) {
  sym.eval(result, variables);
  return result[0];
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

static bool isValidSymbol(std::string_view name) {
  // first char must be a letter or underscore
  if (auto c{name.front()};
      !(std::isalpha(c, std::locale::classic()) || c == '_')) {
    return false;
  }
  // other chars must be letters, numbers or underscores
  return std::all_of(name.begin(), name.end(), [](char c) {
    return std::isalnum(c, std::locale::classic()) || c == '_';
  });
}

static bool isValidDouble(const std::string &str) {
  bool valid{false};
  QString(str.c_str()).toDouble(&valid);
  return valid;
}

static void appendQuotedName(std::string &str, const std::string &name) {
  auto needQuotes{!isValidSymbol(name)};
  if (needQuotes) {
    str.push_back(quoteChar);
  }
  str.append(name);
  if (needQuotes) {
    str.push_back(quoteChar);
  }
}

static void appendQuotedName(QTextCursor &tc, const QString &name) {
  auto needQuotes{!isValidSymbol(name.toStdString())};
  QChar qQuoteChar{quoteChar};
  if (needQuotes) {
    tc.insertText(qQuoteChar);
  }
  tc.insertText(name);
  if (needQuotes) {
    tc.insertText(qQuoteChar);
  }
}

void QPlainTextMathEdit::addVariable(const std::string &variable,
                                     const std::string &displayName) {
  SPDLOG_TRACE("adding var: {}", variable);
  vars.push_back(variable);
  auto name{displayName};
  if (displayName.empty()) {
    name = variable;
  }
  SPDLOG_TRACE("  -> display name {}", name);
  mapDisplayNamesToVars[name] = variable;
  mapVarsToDisplayNames[variable] = name;
  updateCompleter();
  qPlainTextEdit_textChanged();
}

void QPlainTextMathEdit::removeVariable(const std::string &variable) {
  vars.erase(std::remove(vars.begin(), vars.end(), variable), vars.end());
  if (auto iter{mapVarsToDisplayNames.find(variable)};
      iter != mapVarsToDisplayNames.cend()) {
    SPDLOG_TRACE("removing variable: {}", variable);
    const auto &displayName{iter->second};
    SPDLOG_TRACE("  -> display name {}", displayName);
    mapDisplayNamesToVars.erase(displayName);
    mapVarsToDisplayNames.erase(iter);
  }
  qPlainTextEdit_textChanged();
}

void QPlainTextMathEdit::reset() {
  // reset to all built-in sbml L3 math functions and constants
  // http://model.caltech.edu/software/libsbml/5.18.0/docs/formatted/c-api/libsbml-math.html#math-l3
  clearFunctions();
  clearVariables();
  for (const auto &f :
       {"sin",   "cos",      "tan",   "cot",       "csc",   "sec",
        "asin",  "arcsin",   "acos",  "arccos",    "atan",  "arctan",
        "asec",  "arcsec",   "acsc",  "arccsc",    "acot",  "arccot",
        "sinh",  "cosh",     "tanh",  "coth",      "sech",  "csch",
        "asinh", "arcsinh",  "acosh", "arccosh",   "atanh", "arctanh",
        "asech", "arcsech",  "acoth", "arccoth",   "acsch", "arccsch",
        "sqrt",  "abs",      "exp",   "floor",     "ceil",  "ceiling",
        "ln",    "log",      "log10", "factorial", "root",  "sqr",
        "plus",  "minus",    "times", "divide",    "pow",   "power",
        "root",  "max",      "min",   "and",       "or",    "xor",
        "not",   "eq",       "neq",   "geq",       "gt",    "leq",
        "lt",    "piecewise"}) {
    mapDisplayNamesToFuncs[f] = f;
    mapFuncsToDisplayNames[f] = f;
  }
  for (const auto &c : {"pi", "exponentiale", "avogadro", "time", "inf",
                        "infinity", "nan", "notanumber", "true", "false"}) {
    mapDisplayNamesToVars[c] = c;
    mapVarsToDisplayNames[c] = c;
  }
  updateCompleter();
  qPlainTextEdit_textChanged();
}

void QPlainTextMathEdit::addFunction(const sme::utils::Function &function) {
  SPDLOG_TRACE("adding function: {}", function.id);
  functions.push_back(function);
  SPDLOG_TRACE("  -> display name {}", function.name);
  mapDisplayNamesToFuncs[function.name] = function.id;
  mapFuncsToDisplayNames[function.id] = function.name;
  updateCompleter();
  qPlainTextEdit_textChanged();
}

void QPlainTextMathEdit::removeFunction(const std::string &functionId) {
  if (auto iter = mapFuncsToDisplayNames.find(functionId);
      iter != mapFuncsToDisplayNames.cend()) {
    SPDLOG_TRACE("removing function: {}", functionId);
    std::string displayName = iter->second;
    SPDLOG_TRACE("  -> display name {}", displayName);
    mapDisplayNamesToFuncs.erase(displayName);
    mapFuncsToDisplayNames.erase(iter);
  }
  functions.erase(std::remove_if(functions.begin(), functions.end(),
                                 [&functionId](const auto &f) {
                                   return f.id == functionId;
                                 }),
                  functions.end());
  updateCompleter();
  qPlainTextEdit_textChanged();
}

void QPlainTextMathEdit::setConstants(
    const std::vector<sme::model::IdNameValue> &constants) {
  consts.clear();
  consts.reserve(constants.size());
  for (const auto &c : constants) {
    consts.emplace_back(c.id, c.value);
    mapDisplayNamesToVars[c.name] = c.id;
    mapVarsToDisplayNames[c.id] = c.name;
  }
  updateCompleter();
  qPlainTextEdit_textChanged();
}

void QPlainTextMathEdit::updateCompleter() {
  QStringList names;
  for (const auto &[key, value] : mapDisplayNamesToVars) {
    names.push_back(key.c_str());
  }
  for (const auto &[key, value] : mapDisplayNamesToFuncs) {
    names.push_back(key.c_str());
  }
  stringListModel.setStringList(names);
  completer.setModel(&stringListModel);
}

QPlainTextMathEdit::QPlainTextMathEdit(QWidget *parent)
    : QPlainTextEdit(parent) {
  reset();
  completer.setWidget(this);
  completer.setCompletionMode(QCompleter::PopupCompletion);
  completer.setCaseSensitivity(Qt::CaseSensitive);
  connect(&completer, QOverload<const QString &>::of(&QCompleter::activated),
          this, &QPlainTextMathEdit::insertCompletion);
  connect(this, &QPlainTextEdit::textChanged, this,
          &QPlainTextMathEdit::qPlainTextEdit_textChanged);
  connect(this, &QPlainTextEdit::cursorPositionChanged, this,
          &QPlainTextMathEdit::qPlainTextEdit_cursorPositionChanged);
}

// - iterate through each symbol in expr
// - a symbol is any text in quotes or with a delimeter char before and after it
// - a function is a symbol that is directly followed by "("
// - look-up symbol in corresponding map and replace it with result
// - if not found in map return error message
static std::pair<std::string, QString>
substitute(const std::string &expr,
           const std::map<std::string, std::string, std::less<>> &varMap,
           const std::map<std::string, std::string, std::less<>> &funcMap) {
  const std::string delimiters{"-+/()*,^<>&%!|= "};
  SPDLOG_DEBUG("expr: {}", expr);
  if (expr.empty()) {
    return {};
  }
  std::string out;
  out.reserve(expr.size());
  // skip over any initial delimiters
  auto start{expr.find_first_not_of(delimiters)};
  // append them to output
  out.append(expr.substr(0, start));
  std::size_t end;
  std::string var{};
  while (start != std::string::npos) {
    if (expr[start] == quoteChar) {
      // skip starting quote
      ++start;
      // find closing quote char
      end = expr.find(quoteChar, start);
      if (end == std::string::npos) {
        SPDLOG_DEBUG("  - no closing quote");
        return {out, "no closing quote"};
      }
      var = expr.substr(start, end - start);
      // skip closing quote
      ++end;
    } else {
      // find next delimiter
      end = expr.find_first_of(delimiters, start + 1);
      if (std::isdigit(expr[start], std::locale::classic()) &&
          end < expr.size() && expr[end - 1] == 'e' &&
          (expr[end] == '-' || expr[end] == '+')) {
        // symbol starts with a numerical digit, and ends with "e-" or "e+",
        // this can only be valid input if we have the first half of a number in
        // scientific notation, so carry on to next delimiter to get the rest of
        // the number
        end = expr.find_first_of(delimiters, end + 1);
      }
      var = expr.substr(start, end - start);
    }
    SPDLOG_DEBUG("  - var {}", var);
    if (end < expr.size() && expr[end] == '(') {
      // function
      if (auto iter{funcMap.find(var)}; iter != funcMap.cend()) {
        SPDLOG_DEBUG("    -> {} ", iter->second);
        appendQuotedName(out, iter->second);
      } else {
        SPDLOG_DEBUG("    -> function not found");
        return {out, QString("function '%1' not found").arg(var.c_str())};
      }
    } else {
      // variable or number
      if (auto iter{varMap.find(var)}; iter != varMap.cend()) {
        SPDLOG_DEBUG("    -> {} ", iter->second);
        appendQuotedName(out, iter->second);
      } else {
        if (isValidDouble(var)) {
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
    // skip over any delimiters to start of next variable
    start = expr.find_first_not_of(delimiters, end);
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
  return substitute(expr, varMap, funcMap).first;
}

void QPlainTextMathEdit::clearFunctions() {
  mapFuncsToDisplayNames.clear();
  mapDisplayNamesToFuncs.clear();
  functions.clear();
}

std::pair<std::string, QString>
QPlainTextMathEdit::displayNamesToVariables(const std::string &expr) const {
  const auto &varMap = mapDisplayNamesToVars;
  const auto &funcMap = mapDisplayNamesToFuncs;
  if (varMap.empty()) {
    return {expr, ""};
  }
  return substitute(expr, varMap, funcMap);
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
  if (newExpr.empty() && currentErrorMessage.isEmpty()) {
    currentErrorMessage = QString("Empty expression");
  }
  if (currentErrorMessage.isEmpty()) {
    // parse (but don't compile) symbolic expression
    SPDLOG_DEBUG("parsing '{}' with SymEngine backend", newExpr);
    sym = sme::utils::Symbolic(newExpr, vars, consts, functions, false);
    if (sym.isValid()) {
      expressionIsValid = true;
      currentErrorMessage = "";
      currentVariableMath = sym.expr();
      currentDisplayMath = variablesToDisplayNames(currentVariableMath).c_str();
    } else {
      // if SymEngine failed to parse, capture error message
      currentErrorMessage = sym.getErrorMessage().c_str();
    }
  }
  emit mathChanged(currentDisplayMath, expressionIsValid, currentErrorMessage);
}

static std::pair<int, bool> getClosingBracket(const QString &expr, int pos,
                                              int sign) {
  int len = 0;
  int count = sign;
  int iEnd = (sign < 0) ? -1 : static_cast<int>(expr.size());
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

QString QPlainTextMathEdit::getCurrentWord() {
  auto expr{toPlainText()};
  auto tc{textCursor()};
  const auto pos{tc.position()};
  bool inQuotes{false};
  auto quoteEndPos{expr.indexOf(quoteChar)};
  auto quoteStartPos{quoteEndPos};
  while (quoteEndPos >= 0 && quoteEndPos < pos) {
    quoteStartPos = quoteEndPos;
    inQuotes = !inQuotes;
    quoteEndPos = expr.indexOf(quoteChar, quoteStartPos + 1);
  }
  if (inQuotes) {
    // if quoted: word is everything inside the quotes
    currentWordStartPos = quoteStartPos;
    currentWordEndPos = quoteEndPos;
    return expr.mid(quoteStartPos + 1, quoteEndPos - quoteStartPos - 1);
  }
  // otherwise use normal Qt word selection
  tc.select(QTextCursor::WordUnderCursor);
  currentWordStartPos = tc.selectionStart();
  currentWordEndPos = tc.selectionEnd();
  return tc.selectedText();
}

void QPlainTextMathEdit::insertCompletion(const QString &completion) {
  auto tc{textCursor()};
  tc.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor,
                  tc.position() - static_cast<int>(currentWordStartPos));
  if (currentWordEndPos < 0) {
    tc.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
  } else {
    tc.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor,
                    static_cast<int>(currentWordEndPos - currentWordStartPos));
  }
  tc.removeSelectedText();
  appendQuotedName(tc, completion);
  setTextCursor(tc);
}

void QPlainTextMathEdit::keyPressEvent(QKeyEvent *e) {
  if (completer.popup()->isVisible() &&
      (e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return ||
       e->key() == Qt::Key_Escape)) {
    // forward these key events to completer
    e->ignore();
    return;
  }
  // default processing of key event
  QPlainTextEdit::keyPressEvent(e);
  auto word{getCurrentWord()};
  if (word.isEmpty() || word == completer.currentCompletion()) {
    completer.popup()->hide();
    return;
  }
  if (completer.completionPrefix() != word) {
    completer.setCompletionPrefix(word);
    completer.popup()->setCurrentIndex(
        completer.completionModel()->index(0, 0));
  }
  QRect cr = cursorRect();
  cr.setWidth(completer.popup()->sizeHintForColumn(0) +
              completer.popup()->verticalScrollBar()->sizeHint().width());
  completer.complete(cr);
}

#include "dialoganalytic.hpp"

#include <symengine/symengine_exception.h>

#include <QPlainTextEdit>
#include <QPushButton>
#include <QToolTip>

#include "logger.hpp"
#include "ui_dialoganalytic.h"
#include "units.hpp"

DialogAnalytic::DialogAnalytic(const QString& analyticExpression,
                               const sbml::SpeciesGeometry& speciesGeometry,
                               QWidget* parent)
    : QDialog(parent),
      ui(new Ui::DialogAnalytic),
      points(speciesGeometry.compartmentPoints),
      width(speciesGeometry.pixelWidth),
      origin(speciesGeometry.physicalOrigin),
      qpi(speciesGeometry.compartmentImageSize,
          speciesGeometry.compartmentPoints) {
  ui->setupUi(this);

  const auto& units = speciesGeometry.modelUnits;
  lengthUnit = units.getLength().symbol;
  concentrationUnit = QString("%1/%2")
                          .arg(units.getAmount().symbol)
                          .arg(units.getVolume().symbol);
  img = QImage(speciesGeometry.compartmentImageSize,
               QImage::Format_ARGB32_Premultiplied);
  img.fill(0);
  concentration.resize(points.size(), 0.0);

  connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
          &DialogAnalytic::accept);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
          &DialogAnalytic::reject);
  connect(ui->txtExpression, &QPlainTextEdit::textChanged, this,
          &DialogAnalytic::txtExpression_textChanged);
  connect(ui->txtExpression, &QPlainTextEdit::cursorPositionChanged, this,
          &DialogAnalytic::txtExpression_cursorPositionChanged);
  connect(ui->lblImage, &QLabelMouseTracker::mouseOver, this,
          &DialogAnalytic::lblImage_mouseOver);

  ui->txtExpression->setPlainText(analyticExpression);
  lblImage_mouseOver(points.front());
}

const std::string& DialogAnalytic::getExpression() const { return expression; }

bool DialogAnalytic::isExpressionValid() const { return expressionIsValid; }

QPointF DialogAnalytic::physicalPoint(const QPoint& pixelPoint) const {
  // position in pixels (with (0,0) in top-left of image)
  // rescale to physical x,y point (with (0,0) in bottom-left)
  QPointF physical;
  physical.setX(origin.x() + width * static_cast<double>(pixelPoint.x()));
  physical.setY(origin.x() +
                width * static_cast<double>(img.height() - 1 - pixelPoint.y()));
  return physical;
}

static std::pair<int, QColor> getClosingBracket(const QString& expr, int pos,
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
      // green: found closing bracket
      return {len + 1, QColor(200, 255, 200)};
    }
  }
  // red: did not find closing bracket
  return {len + 1, QColor(255, 150, 150)};
}

void DialogAnalytic::txtExpression_cursorPositionChanged() {
  // very basic syntax highlighting:
  // if cursor is before a '(', highlight with green until closing bracket
  // if there is no closing bracket, highlight until end with red
  auto expr = ui->txtExpression->toPlainText();
  int i = ui->txtExpression->textCursor().position();
  QTextEdit::ExtraSelection s;
  s.cursor = ui->txtExpression->textCursor();
  if (expr[i] == '(') {
    const auto& [len, col] = getClosingBracket(expr, i, +1);
    s.cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor,
                          len);
    s.format.setBackground(QBrush(col));
  } else if (i > 0 && expr[i - 1] == ')') {
    const auto& [len, col] = getClosingBracket(expr, i - 1, -1);
    s.cursor.movePosition(QTextCursor::PreviousCharacter,
                          QTextCursor::KeepAnchor, len);
    s.format.setBackground(QBrush(col));
  }
  ui->txtExpression->setExtraSelections({s});
}

void DialogAnalytic::txtExpression_textChanged() {
  expressionIsValid = false;
  ui->lblConcentration->setText("");
  expression.clear();
  // check for illegal chars
  std::string newExpr = ui->txtExpression->toPlainText().toStdString();
  if (newExpr.find_first_of("%@&!") != std::string::npos) {
    ui->lblExpressionStatus->setText("expression contains illegal character");
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    return;
  }
  try {
    // parse symbolic expression
    symbolic::Symbolic sym(newExpr, {"x", "y"}, {});
    // calculate concentration
    auto res = std::vector<double>(1, 0);
    auto vars = std::vector<double>(2, 0);
    for (std::size_t i = 0; i < points.size(); ++i) {
      auto physical = physicalPoint(points[i]);
      vars[0] = physical.x();
      vars[1] = physical.y();
      sym.evalLLVM(res, vars);
      concentration[i] = res[0];
    }
    // check for negative values
    if (*std::min_element(concentration.cbegin(), concentration.cend()) < 0) {
      ui->lblExpressionStatus->setText("concentration cannot be negative");
    } else {
      ui->lblExpressionStatus->setText("");
      expressionIsValid = true;
    }
    // construct image of concentration
    if (expressionIsValid) {
      expression = sym.simplify();
      // normalise intensity to max concentration
      double maxConc =
          *std::max_element(concentration.cbegin(), concentration.cend());
      for (std::size_t i = 0; i < points.size(); ++i) {
        int intensity = static_cast<int>(255 * concentration[i] / maxConc);
        img.setPixel(points[i], QColor(intensity, intensity, intensity).rgb());
      }
    } else {
      img.fill(0);
    }
    ui->lblImage->setImage(img);
  } catch (SymEngine::SymEngineException& e) {
    // if SymEngine failed to parse, show error message
    img.fill(0);
    ui->lblImage->setImage(img);
    ui->lblExpressionStatus->setText(e.what());
  }
  ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(expressionIsValid);
  return;
}

void DialogAnalytic::lblImage_mouseOver(QPoint point) {
  if (!expressionIsValid) {
    return;
  }
  auto index = qpi.getIndex(point);
  if (!index) {
    ui->lblConcentration->setText("");
    return;
  }
  auto physical = physicalPoint(point);
  ui->lblConcentration->setText(QString("x=%1 %2, y=%3 %2, concentration=%4 %5")
                                    .arg(physical.x())
                                    .arg(lengthUnit)
                                    .arg(physical.y())
                                    .arg(concentration[*index])
                                    .arg(concentrationUnit));
}

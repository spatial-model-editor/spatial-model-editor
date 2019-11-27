#include <QtTest>

#include "catch_wrapper.hpp"
#include "logger.hpp"
#include "qplaintextmathedit.hpp"
#include "qt_test_utils.hpp"

SCENARIO("QPlainTextMathEdit", "[qplaintextmathedit][gui]") {
  QPlainTextMathEdit mathEdit;
  struct Signal {
    QString math;
    QString error;
    bool valid;
  };
  Signal signal;
  QObject::connect(
      &mathEdit, &QPlainTextMathEdit::mathChanged,
      [&signal](const QString &math, bool valid, const QString &errorMessage) {
        signal = {math, errorMessage, valid};
      });
  REQUIRE(mathEdit.getMath().toStdString() == "");
  REQUIRE(mathEdit.mathIsValid() == false);
  REQUIRE(mathEdit.getErrorMessage().toStdString() == "");
  GIVEN("expression and/or variables") {
    mathEdit.show();
    // "1"
    QTest::keyClick(&mathEdit, Qt::Key_1);
    REQUIRE(mathEdit.getMath().toStdString() == "1");
    REQUIRE(mathEdit.mathIsValid() == true);
    REQUIRE(mathEdit.getErrorMessage().toStdString() == "");
    REQUIRE(signal.math == mathEdit.getMath());
    REQUIRE(signal.valid == mathEdit.mathIsValid());
    REQUIRE(signal.error == mathEdit.getErrorMessage());
    mathEdit.compileMath();
    REQUIRE(mathEdit.evaluateMath() == dbl_approx(1.0));

    // "1*"
    QTest::keyClick(&mathEdit, Qt::Key_Asterisk);
    REQUIRE(mathEdit.getMath().toStdString() == "");
    REQUIRE(mathEdit.mathIsValid() == false);
    REQUIRE(mathEdit.getErrorMessage().toStdString() == "syntax error");
    REQUIRE(signal.math == mathEdit.getMath());
    REQUIRE(signal.valid == mathEdit.mathIsValid());
    REQUIRE(signal.error == mathEdit.getErrorMessage());

    // "1*2"
    QTest::keyClick(&mathEdit, Qt::Key_2);
    REQUIRE(mathEdit.getMath().toStdString() == "2");
    REQUIRE(mathEdit.mathIsValid() == true);
    REQUIRE(mathEdit.getErrorMessage().toStdString() == "");
    REQUIRE(signal.math == mathEdit.getMath());
    REQUIRE(signal.valid == mathEdit.mathIsValid());
    REQUIRE(signal.error == mathEdit.getErrorMessage());
    mathEdit.compileMath();
    REQUIRE(mathEdit.evaluateMath() == dbl_approx(2.0));

    // "1*2+x" (x not a variable)
    QTest::keyClick(&mathEdit, Qt::Key_Plus);
    QTest::keyClick(&mathEdit, Qt::Key_X);
    REQUIRE(mathEdit.getMath().toStdString() == "");
    REQUIRE(mathEdit.mathIsValid() == false);
    REQUIRE(mathEdit.getErrorMessage().toStdString() == "Unknown symbol: x");
    REQUIRE(signal.math == mathEdit.getMath());
    REQUIRE(signal.valid == mathEdit.mathIsValid());
    REQUIRE(signal.error == mathEdit.getErrorMessage());
    mathEdit.addVariable("x");
    REQUIRE(mathEdit.getMath().toStdString() == "2 + x");
    REQUIRE(mathEdit.mathIsValid() == true);
    REQUIRE(mathEdit.getErrorMessage().toStdString() == "");
    REQUIRE(signal.math == mathEdit.getMath());
    REQUIRE(signal.valid == mathEdit.mathIsValid());
    REQUIRE(signal.error == mathEdit.getErrorMessage());
    mathEdit.addVariable("y");
    // removing non-existent variable is a no-op
    mathEdit.removeVariable("notfound");
    mathEdit.removeVariable("x");
    REQUIRE(mathEdit.getMath().toStdString() == "");
    REQUIRE(mathEdit.mathIsValid() == false);
    REQUIRE(mathEdit.getErrorMessage().toStdString() == "Unknown symbol: x");
    REQUIRE(signal.math == mathEdit.getMath());
    REQUIRE(signal.valid == mathEdit.mathIsValid());
    REQUIRE(signal.error == mathEdit.getErrorMessage());

    // "1*2+x&" (& not a valid character)
    QTest::keyClick(&mathEdit, Qt::Key_Ampersand);
    REQUIRE(mathEdit.getMath().toStdString() == "");
    REQUIRE(mathEdit.mathIsValid() == false);
    REQUIRE(mathEdit.getErrorMessage().toStdString() == "Illegal character: &");
    REQUIRE(signal.math == mathEdit.getMath());
    REQUIRE(signal.valid == mathEdit.mathIsValid());
    REQUIRE(signal.error == mathEdit.getErrorMessage());

    // "1*2+x!" (! not a valid character)
    QTest::keyClick(&mathEdit, Qt::Key_Backspace);
    QTest::keyClick(&mathEdit, Qt::Key_Exclam);
    REQUIRE(mathEdit.getMath().toStdString() == "");
    REQUIRE(mathEdit.mathIsValid() == false);
    REQUIRE(mathEdit.getErrorMessage().toStdString() == "Illegal character: !");
    REQUIRE(signal.math == mathEdit.getMath());
    REQUIRE(signal.valid == mathEdit.mathIsValid());
    REQUIRE(signal.error == mathEdit.getErrorMessage());

    // "1*2+5*(1+3)"
    QTest::keyClick(&mathEdit, Qt::Key_Backspace);
    QTest::keyClick(&mathEdit, Qt::Key_Backspace);
    QTest::keyClick(&mathEdit, Qt::Key_5);
    QTest::keyClick(&mathEdit, Qt::Key_Asterisk);
    QTest::keyClick(&mathEdit, Qt::Key_ParenLeft);
    QTest::keyClick(&mathEdit, Qt::Key_1);
    QTest::keyClick(&mathEdit, Qt::Key_Plus);
    QTest::keyClick(&mathEdit, Qt::Key_3);
    QTest::keyClick(&mathEdit, Qt::Key_ParenRight);
    REQUIRE(mathEdit.getMath().toStdString() == "22");
    REQUIRE(mathEdit.mathIsValid() == true);
    REQUIRE(mathEdit.getErrorMessage().toStdString() == "");
    REQUIRE(signal.math == mathEdit.getMath());
    REQUIRE(signal.valid == mathEdit.mathIsValid());
    REQUIRE(signal.error == mathEdit.getErrorMessage());
    mathEdit.compileMath();
    REQUIRE(mathEdit.evaluateMath() == dbl_approx(22.0));
  }
  GIVEN("expression with variables") {
    mathEdit.show();
    REQUIRE(mathEdit.getVariables().empty() == true);
    mathEdit.setVariables({"x", "y"});
    REQUIRE(mathEdit.getVariables().size() == 2);
    REQUIRE(mathEdit.getVariables()[0] == "x");
    REQUIRE(mathEdit.getVariables()[1] == "y");
    mathEdit.addVariable("z");
    mathEdit.removeVariable("x");
    REQUIRE(mathEdit.getVariables().size() == 2);
    REQUIRE(mathEdit.getVariables()[0] == "y");
    REQUIRE(mathEdit.getVariables()[1] == "z");
    mathEdit.addVariable("x");
    REQUIRE(mathEdit.getVariables().size() == 3);
    REQUIRE(mathEdit.getVariables()[0] == "y");
    REQUIRE(mathEdit.getVariables()[1] == "z");
    REQUIRE(mathEdit.getVariables()[2] == "x");
    mathEdit.removeVariable("y");
    mathEdit.removeVariable("z");
    mathEdit.addVariable("y");
    REQUIRE(mathEdit.getVariables().size() == 2);
    REQUIRE(mathEdit.getVariables()[0] == "x");
    REQUIRE(mathEdit.getVariables()[1] == "y");
    REQUIRE(mathEdit.getMath().toStdString() == "");
    REQUIRE(mathEdit.mathIsValid() == false);
    REQUIRE(mathEdit.getErrorMessage().toStdString() == "syntax error");
    // x+2*y
    QTest::keyClick(&mathEdit, Qt::Key_X);
    QTest::keyClick(&mathEdit, Qt::Key_Plus);
    QTest::keyClick(&mathEdit, Qt::Key_2);
    QTest::keyClick(&mathEdit, Qt::Key_Asterisk);
    QTest::keyClick(&mathEdit, Qt::Key_Y);
    REQUIRE(mathEdit.getMath().toStdString() == "x + 2*y");
    REQUIRE(mathEdit.mathIsValid() == true);
    REQUIRE(mathEdit.getErrorMessage().toStdString() == "");
    REQUIRE(signal.math == mathEdit.getMath());
    REQUIRE(signal.valid == mathEdit.mathIsValid());
    REQUIRE(signal.error == mathEdit.getErrorMessage());
    mathEdit.compileMath();
    REQUIRE(mathEdit.evaluateMath({0.0, 0.0}) == dbl_approx(0.0));
    REQUIRE(mathEdit.evaluateMath({1.0, 0.0}) == dbl_approx(1.0));
    REQUIRE(mathEdit.evaluateMath({0.0, 1.0}) == dbl_approx(2.0));
    REQUIRE(mathEdit.evaluateMath({1.0, 1.0}) == dbl_approx(3.0));
  }
}

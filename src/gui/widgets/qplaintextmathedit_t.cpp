#include "catch_wrapper.hpp"
#include "logger.hpp"
#include "qplaintextmathedit.hpp"
#include "qt_test_utils.hpp"

SCENARIO("QPlainTextMathEdit", "[gui/widgets/qplaintextmathedit][gui/"
                               "widgets][gui][qplaintextmathedit][symbolic]") {
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
  REQUIRE(mathEdit.getMath() == "");
  REQUIRE(mathEdit.mathIsValid() == false);
  REQUIRE(mathEdit.getErrorMessage() == "");
  GIVEN("expression and/or variables") {
    mathEdit.show();
    // "1"
    sendKeyEvents(&mathEdit, {"1"});
    REQUIRE(mathEdit.getMath() == "1");
    REQUIRE(mathEdit.mathIsValid() == true);
    REQUIRE(mathEdit.getErrorMessage() == "");
    REQUIRE(signal.math == mathEdit.getMath());
    REQUIRE(signal.valid == mathEdit.mathIsValid());
    REQUIRE(signal.error == mathEdit.getErrorMessage());
    mathEdit.compileMath();
    REQUIRE(mathEdit.evaluateMath() == dbl_approx(1.0));

    // "1*"
    sendKeyEvents(&mathEdit, {"*"});
    REQUIRE(mathEdit.getMath() == "");
    REQUIRE(mathEdit.mathIsValid() == false);
    REQUIRE(mathEdit.getErrorMessage() == "syntax error");
    REQUIRE(signal.math == mathEdit.getMath());
    REQUIRE(signal.valid == mathEdit.mathIsValid());
    REQUIRE(signal.error == mathEdit.getErrorMessage());

    // "1*2"
    sendKeyEvents(&mathEdit, {"2"});
    REQUIRE(mathEdit.getMath() == "2");
    REQUIRE(mathEdit.mathIsValid() == true);
    REQUIRE(mathEdit.getErrorMessage() == "");
    REQUIRE(signal.math == mathEdit.getMath());
    REQUIRE(signal.valid == mathEdit.mathIsValid());
    REQUIRE(signal.error == mathEdit.getErrorMessage());
    mathEdit.compileMath();
    REQUIRE(mathEdit.evaluateMath() == dbl_approx(2.0));

    // "1*2+x" (x not a variable)
    sendKeyEvents(&mathEdit, {"+", "x"});
    REQUIRE(mathEdit.getMath() == "");
    REQUIRE(mathEdit.mathIsValid() == false);
    REQUIRE(mathEdit.getErrorMessage() == "Unknown symbol: x");
    REQUIRE(signal.math == mathEdit.getMath());
    REQUIRE(signal.valid == mathEdit.mathIsValid());
    REQUIRE(signal.error == mathEdit.getErrorMessage());
    mathEdit.addVariable("x");
    REQUIRE(mathEdit.getMath() == "2 + x");
    REQUIRE(mathEdit.mathIsValid() == true);
    REQUIRE(mathEdit.getErrorMessage() == "");
    REQUIRE(signal.math == mathEdit.getMath());
    REQUIRE(signal.valid == mathEdit.mathIsValid());
    REQUIRE(signal.error == mathEdit.getErrorMessage());
    mathEdit.addVariable("y");
    // removing non-existent variable is a no-op
    mathEdit.removeVariable("notfound");
    mathEdit.removeVariable("x");
    REQUIRE(mathEdit.getMath() == "");
    REQUIRE(mathEdit.mathIsValid() == false);
    REQUIRE(mathEdit.getErrorMessage() == "name 'x' not found");
    REQUIRE(signal.math == mathEdit.getMath());
    REQUIRE(signal.valid == mathEdit.mathIsValid());
    REQUIRE(signal.error == mathEdit.getErrorMessage());

    // "1*2+x&" (& not a valid character)
    sendKeyEvents(&mathEdit, {"&"});
    REQUIRE(mathEdit.getMath() == "");
    REQUIRE(mathEdit.mathIsValid() == false);
    REQUIRE(mathEdit.getErrorMessage() == "name 'x&' not found");
    REQUIRE(signal.math == mathEdit.getMath());
    REQUIRE(signal.valid == mathEdit.mathIsValid());
    REQUIRE(signal.error == mathEdit.getErrorMessage());

    // "1*2+x!" (! not a valid character)
    sendKeyEvents(&mathEdit, {"Backspace", "!"});
    REQUIRE(mathEdit.getMath() == "");
    REQUIRE(mathEdit.mathIsValid() == false);
    REQUIRE(mathEdit.getErrorMessage() == "name 'x!' not found");
    REQUIRE(signal.math == mathEdit.getMath());
    REQUIRE(signal.valid == mathEdit.mathIsValid());
    REQUIRE(signal.error == mathEdit.getErrorMessage());

    // "1*2+5*(1+3)"
    sendKeyEvents(&mathEdit, {"Backspace", "Backspace", "5", "*", "(", "1", "+",
                              "3", ")"});
    REQUIRE(mathEdit.getMath() == "22");
    REQUIRE(mathEdit.mathIsValid() == true);
    REQUIRE(mathEdit.getErrorMessage() == "");
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
    REQUIRE(mathEdit.getMath() == "");
    REQUIRE(mathEdit.mathIsValid() == false);
    REQUIRE(mathEdit.getErrorMessage() == "Empty expression");
    // x+2*y
    sendKeyEvents(&mathEdit, {"x", "+", "2", "*", "y"});
    REQUIRE(mathEdit.getMath() == "x + 2*y");
    REQUIRE(mathEdit.mathIsValid() == true);
    REQUIRE(mathEdit.getErrorMessage() == "");
    REQUIRE(signal.math == mathEdit.getMath());
    REQUIRE(signal.valid == mathEdit.mathIsValid());
    REQUIRE(signal.error == mathEdit.getErrorMessage());
    mathEdit.compileMath();
    REQUIRE(mathEdit.evaluateMath({0.0, 0.0}) == dbl_approx(0.0));
    REQUIRE(mathEdit.evaluateMath({1.0, 0.0}) == dbl_approx(1.0));
    REQUIRE(mathEdit.evaluateMath({0.0, 1.0}) == dbl_approx(2.0));
    REQUIRE(mathEdit.evaluateMath({1.0, 1.0}) == dbl_approx(3.0));
    mathEdit.clearVariables();
    REQUIRE(mathEdit.getVariables().empty() == true);
  }
  GIVEN("expression with unquoted display names") {
    mathEdit.show();
    mathEdit.clearVariables();
    REQUIRE(mathEdit.getVariables().empty() == true);
    // "X + y_var"
    sendKeyEvents(&mathEdit, {"X", "+", "y", "_", "v", "a", "r"});
    REQUIRE(mathEdit.mathIsValid() == false);
    REQUIRE(mathEdit.getErrorMessage() == "Unknown symbol: X");
    mathEdit.addVariable("x", "X");
    REQUIRE(mathEdit.mathIsValid() == false);
    REQUIRE(mathEdit.getErrorMessage() == "name 'y_var' not found");
    mathEdit.addVariable("y", "y_var");
    REQUIRE(mathEdit.getVariables().size() == 2);
    REQUIRE(mathEdit.mathIsValid() == true);
    REQUIRE(signal.math == "X + y_var");
    REQUIRE(mathEdit.getVariableMath() == "x + y");
    // "X + y_var + 3*X"
    sendKeyEvents(&mathEdit, {"+", "3", "*", "X"});
    REQUIRE(mathEdit.mathIsValid() == true);
    REQUIRE(signal.math == "4*X + y_var");
    REQUIRE(mathEdit.getVariableMath() == "4*x + y");
    mathEdit.removeVariable("y");
    REQUIRE(mathEdit.mathIsValid() == false);
    REQUIRE(mathEdit.getErrorMessage() == "name 'y_var' not found");
    mathEdit.removeVariable("x");
    REQUIRE(mathEdit.getVariableMath().empty() == true);
    REQUIRE(mathEdit.mathIsValid() == false);
    REQUIRE(mathEdit.getErrorMessage() == "Unknown symbol: X");
  }
  GIVEN("expression with quoted display names") {
    mathEdit.show();
    REQUIRE(mathEdit.getVariables().empty() == true);
    mathEdit.addVariable("x", "X var!");
    REQUIRE(mathEdit.getVariables().size() == 1);
    // "X var!"
    sendKeyEvents(&mathEdit, {"\"", "X", " ", "v", "a", "r", "!", "\""});
    REQUIRE(signal.math == "\"X var!\"");
    REQUIRE(mathEdit.getVariableMath() == "x");
    // "X var!" + "X var!"
    sendKeyEvents(&mathEdit, {"+", "\"", "X", " ", "v", "a", "r", "!", "\""});
    REQUIRE(signal.math == "2*\"X var!\"");
    REQUIRE(mathEdit.getVariableMath() == "2*x");
    mathEdit.removeVariable("x");
    REQUIRE(mathEdit.mathIsValid() == false);
    REQUIRE(mathEdit.getErrorMessage() == "Illegal character: !");
  }
  GIVEN("expression with user-defined function") {
    mathEdit.show();
    REQUIRE(mathEdit.getVariables().empty() == true);
    mathEdit.addVariable("x", "X");
    REQUIRE(mathEdit.getVariables().size() == 1);
    // "cos(X)"
    sendKeyEvents(&mathEdit, {"c", "o", "s", "(", "X", ")"});
    REQUIRE(signal.math == "");
    REQUIRE(mathEdit.getVariableMath().empty() == true);
    REQUIRE(mathEdit.getErrorMessage() == "name 'cos' not found");
    REQUIRE(mathEdit.mathIsValid() == false);
    mathEdit.addVariable("cos");
    REQUIRE(mathEdit.getVariables().size() == 2);
    REQUIRE(signal.math == "cos(X)");
    REQUIRE(mathEdit.getVariableMath() == "cos(x)");
    REQUIRE(mathEdit.getErrorMessage() == "");
    REQUIRE(mathEdit.mathIsValid() == true);
  }
}

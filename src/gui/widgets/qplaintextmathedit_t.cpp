#include "catch_wrapper.hpp"
#include "qplaintextmathedit.hpp"
#include "qt_test_utils.hpp"
#include <QApplication>

using namespace sme::test;

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
  REQUIRE(mathEdit.getErrorMessage() == "Empty expression");
  WHEN("SymEngine backend") {
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

      // "1*2+q" (q not a variable)
      sendKeyEvents(&mathEdit, {"+", "q"});
      REQUIRE(mathEdit.getMath() == "");
      REQUIRE(mathEdit.mathIsValid() == false);
      REQUIRE(mathEdit.getErrorMessage() == "variable 'q' not found");
      REQUIRE(signal.math == mathEdit.getMath());
      REQUIRE(signal.valid == mathEdit.mathIsValid());
      REQUIRE(signal.error == mathEdit.getErrorMessage());
      mathEdit.addVariable("q");
      REQUIRE(mathEdit.getMath() == "2 + q");
      REQUIRE(mathEdit.mathIsValid() == true);
      REQUIRE(mathEdit.getErrorMessage() == "");
      REQUIRE(signal.math == mathEdit.getMath());
      REQUIRE(signal.valid == mathEdit.mathIsValid());
      REQUIRE(signal.error == mathEdit.getErrorMessage());
      mathEdit.addVariable("y");
      // removing non-existent variable is a no-op
      mathEdit.removeVariable("notfound");
      mathEdit.removeVariable("q");
      REQUIRE(mathEdit.getMath() == "");
      REQUIRE(mathEdit.mathIsValid() == false);
      REQUIRE(mathEdit.getErrorMessage() == "variable 'q' not found");
      REQUIRE(signal.math == mathEdit.getMath());
      REQUIRE(signal.valid == mathEdit.mathIsValid());
      REQUIRE(signal.error == mathEdit.getErrorMessage());

      // "1*2+5*(1+3)"
      sendKeyEvents(&mathEdit,
                    {"Backspace", "+", "5", "*", "(", "1", "+", "3", ")"});
      REQUIRE(mathEdit.getMath() == "22");
      REQUIRE(mathEdit.mathIsValid() == true);
      REQUIRE(mathEdit.getErrorMessage() == "");
      REQUIRE(signal.math == mathEdit.getMath());
      REQUIRE(signal.valid == mathEdit.mathIsValid());
      REQUIRE(signal.error == mathEdit.getErrorMessage());
      mathEdit.compileMath();
      REQUIRE(mathEdit.evaluateMath() == dbl_approx(22.0));
    }
    GIVEN("scientific notation numbers") {
      mathEdit.show();
      // "-3e-12"
      sendKeyEvents(&mathEdit, {"-", "3", "e", "-", "1", "2"});
      REQUIRE(mathEdit.getMath() == "-3e-12");
      REQUIRE(mathEdit.mathIsValid() == true);
      REQUIRE(mathEdit.getErrorMessage() == "");
      REQUIRE(signal.math == mathEdit.getMath());
      REQUIRE(signal.valid == mathEdit.mathIsValid());
      REQUIRE(signal.error == mathEdit.getErrorMessage());
      mathEdit.compileMath();
      REQUIRE(mathEdit.evaluateMath() == dbl_approx(-3e-12));
      // "-3e-12+"
      sendKeyEvents(&mathEdit, {"+"});
      REQUIRE(mathEdit.getMath() == "");
      REQUIRE(mathEdit.mathIsValid() == false);
      REQUIRE(mathEdit.getErrorMessage() == "syntax error");
      REQUIRE(signal.math == mathEdit.getMath());
      REQUIRE(signal.valid == mathEdit.mathIsValid());
      REQUIRE(signal.error == mathEdit.getErrorMessage());
      // "-3e-12+1"
      sendKeyEvents(&mathEdit, {"1"});
      REQUIRE(mathEdit.getMath() == "0.999999999997");
      REQUIRE(mathEdit.mathIsValid() == true);
      REQUIRE(mathEdit.getErrorMessage() == "");
      REQUIRE(signal.math == mathEdit.getMath());
      REQUIRE(signal.valid == mathEdit.mathIsValid());
      REQUIRE(signal.error == mathEdit.getErrorMessage());
      mathEdit.compileMath();
      REQUIRE(mathEdit.evaluateMath() == dbl_approx(1 - 3e-12));
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
      REQUIRE(mathEdit.evaluateMath(std::vector<double>{0.0, 0.0}) ==
              dbl_approx(0.0));
      REQUIRE(mathEdit.evaluateMath(std::vector<double>{1.0, 0.0}) ==
              dbl_approx(1.0));
      REQUIRE(mathEdit.evaluateMath(std::vector<double>{0.0, 1.0}) ==
              dbl_approx(2.0));
      REQUIRE(mathEdit.evaluateMath(std::vector<double>{1.0, 1.0}) ==
              dbl_approx(3.0));
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
      REQUIRE(mathEdit.getErrorMessage() == "variable 'y_var' not found");
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
      REQUIRE(mathEdit.getErrorMessage() == "variable 'y_var' not found");
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
      // "X var!" + "X var!" (using autocomplete incl. quoting for second one)
      sendKeyEvents(&mathEdit, {"+", "\"", "X", " ", "v"});
      sendKeyEvents(QApplication::activePopupWidget(), {"Enter"});
      REQUIRE(signal.math == "2*\"X var!\"");
      REQUIRE(mathEdit.getVariableMath() == "2*x");
      mathEdit.removeVariable("x");
      REQUIRE(mathEdit.mathIsValid() == false);
      REQUIRE(mathEdit.getErrorMessage() == "variable 'X var!' not found");
    }
    GIVEN("expression with user-defined function") {
      mathEdit.show();
      REQUIRE(mathEdit.getVariables().empty() == true);
      mathEdit.addVariable("x", "X");
      REQUIRE(mathEdit.getVariables().size() == 1);
      // "cse(X)"
      sendKeyEvents(&mathEdit, {"c", "s", "e", "(", "X", ")"});
      REQUIRE(signal.math == "");
      REQUIRE(mathEdit.getVariableMath().empty() == true);
      REQUIRE(mathEdit.getErrorMessage() == "function 'cse' not found");
      REQUIRE(mathEdit.mathIsValid() == false);
      sme::utils::Function f;
      f.id = "cse";
      f.name = "cse";
      f.args = {"z"};
      f.body = "cos(z)";
      mathEdit.addFunction(f);
      REQUIRE(mathEdit.getVariables().size() == 1);
      REQUIRE(signal.math == "cse(X)");
      REQUIRE(mathEdit.getVariableMath() == "cse(x)");
      REQUIRE(mathEdit.getErrorMessage() == "");
      REQUIRE(mathEdit.mathIsValid() == true);
    }
  }
}

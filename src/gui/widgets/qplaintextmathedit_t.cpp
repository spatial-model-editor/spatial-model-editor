#include "catch_wrapper.hpp"
#include "logger.hpp"
#include "model.hpp"
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
      REQUIRE(mathEdit.getErrorMessage() == "variable 'x' not found");
      REQUIRE(signal.math == mathEdit.getMath());
      REQUIRE(signal.valid == mathEdit.mathIsValid());
      REQUIRE(signal.error == mathEdit.getErrorMessage());

      // "1*2+x&" (& not a valid character)
      sendKeyEvents(&mathEdit, {"&"});
      REQUIRE(mathEdit.getMath() == "");
      REQUIRE(mathEdit.mathIsValid() == false);
      REQUIRE(mathEdit.getErrorMessage() == "variable 'x&' not found");
      REQUIRE(signal.math == mathEdit.getMath());
      REQUIRE(signal.valid == mathEdit.mathIsValid());
      REQUIRE(signal.error == mathEdit.getErrorMessage());

      // "1*2+x!" (! not a valid character)
      sendKeyEvents(&mathEdit, {"Backspace", "!"});
      REQUIRE(mathEdit.getMath() == "");
      REQUIRE(mathEdit.mathIsValid() == false);
      REQUIRE(mathEdit.getErrorMessage() == "variable 'x!' not found");
      REQUIRE(signal.math == mathEdit.getMath());
      REQUIRE(signal.valid == mathEdit.mathIsValid());
      REQUIRE(signal.error == mathEdit.getErrorMessage());

      // "1*2+5*(1+3)"
      sendKeyEvents(&mathEdit, {"Backspace", "Backspace", "5", "*", "(", "1",
                                "+", "3", ")"});
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
      // using libSBML map interface for variable values works but gives logger
      // warning about inefficiency
      std::map<const std::string, std::pair<double, bool>> vars;
      vars["x"] = {1.0, false};
      vars["y"] = {1.0, false};
      REQUIRE(mathEdit.evaluateMath(vars) == dbl_approx(3.0));
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
      // "cse(X)"
      sendKeyEvents(&mathEdit, {"c", "s", "e", "(", "X", ")"});
      REQUIRE(signal.math == "");
      REQUIRE(mathEdit.getVariableMath().empty() == true);
      REQUIRE(mathEdit.getErrorMessage() == "function 'cse' not found");
      REQUIRE(mathEdit.mathIsValid() == false);
      mathEdit.addFunction("cse");
      REQUIRE(mathEdit.getVariables().size() == 1);
      REQUIRE(signal.math == "cse(X)");
      REQUIRE(mathEdit.getVariableMath() == "cse(x)");
      REQUIRE(mathEdit.getErrorMessage() == "");
      REQUIRE(mathEdit.mathIsValid() == true);
    }
  }
  WHEN("libSBML backend") {
    model::Model model;
    QFile f(":/models/ABtoC.xml");
    f.open(QIODevice::ReadOnly);
    model.importSBMLString(f.readAll().toStdString());
    mathEdit.enableLibSbmlBackend(&model.getMath());
    GIVEN("expression without variables") {
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
      REQUIRE(mathEdit.getErrorMessage() ==
              "Error when parsing input '1*' at position 2:  syntax error, "
              "unexpected end of string");
      REQUIRE(signal.math == mathEdit.getMath());
      REQUIRE(signal.valid == mathEdit.mathIsValid());
      REQUIRE(signal.error == mathEdit.getErrorMessage());

      // "1*2"
      sendKeyEvents(&mathEdit, {"2"});
      REQUIRE(mathEdit.getMath() == "1*2");
      REQUIRE(mathEdit.mathIsValid() == true);
      REQUIRE(mathEdit.getErrorMessage() == "");
      REQUIRE(signal.math == mathEdit.getMath());
      REQUIRE(signal.valid == mathEdit.mathIsValid());
      REQUIRE(signal.error == mathEdit.getErrorMessage());
      mathEdit.compileMath();
      REQUIRE(mathEdit.evaluateMath() == dbl_approx(2.0));

      // "1*2+cos(0)"
      sendKeyEvents(&mathEdit, {"+", "c", "o", "s", "(", "0", ")"});
      REQUIRE(mathEdit.getMath() == "1*2+cos(0)");
      REQUIRE(mathEdit.mathIsValid() == true);
      REQUIRE(mathEdit.getErrorMessage() == "");
      REQUIRE(signal.math == mathEdit.getMath());
      REQUIRE(signal.valid == mathEdit.mathIsValid());
      REQUIRE(signal.error == mathEdit.getErrorMessage());
      mathEdit.compileMath();
      REQUIRE(mathEdit.evaluateMath() == dbl_approx(3.0));
    }
    GIVEN("expression with implicit variable (that exists in model)") {
      mathEdit.show();
      // "1+cos(x)"
      sendKeyEvents(&mathEdit, {"1", "+", "c", "o", "s", "(", "x", ")"});
      REQUIRE(mathEdit.getMath() == "1+cos(x)");
      REQUIRE(mathEdit.mathIsValid() == true);
      REQUIRE(mathEdit.getErrorMessage() == "");
      REQUIRE(signal.math == mathEdit.getMath());
      REQUIRE(signal.valid == mathEdit.mathIsValid());
      REQUIRE(signal.error == mathEdit.getErrorMessage());
      mathEdit.compileMath();
      std::map<const std::string, std::pair<double, bool>> vars;
      // if we don't specify value for x, get nan
      REQUIRE(std::isnan(mathEdit.evaluateMath(vars)));
      vars["x"] = {0.0, false};
      REQUIRE(mathEdit.evaluateMath(vars) == dbl_approx(2.0));
    }
    GIVEN("expression with implicit variable (that doesn't exist in model)") {
      mathEdit.show();
      // "1+cos(z)"
      sendKeyEvents(&mathEdit, {"1", "+", "c", "o", "s", "(", "z", ")"});
      REQUIRE(mathEdit.getMath() == "");
      REQUIRE(mathEdit.mathIsValid() == false);
      REQUIRE(mathEdit.getErrorMessage() == "Unknown variable: z");
      REQUIRE(signal.math == mathEdit.getMath());
      REQUIRE(signal.valid == mathEdit.mathIsValid());
      REQUIRE(signal.error == mathEdit.getErrorMessage());
    }
    GIVEN("expression with user defined function (that exists in model)") {
      model.getFunctions().add("qwq");
      model.getFunctions().addArgument("qwq", "x");
      model.getFunctions().setExpression("qwq", "2*x");
      mathEdit.show();
      // "1+qwq(x)"
      sendKeyEvents(&mathEdit, {"1", "+", "q", "w", "q", "(", "x", ")"});
      REQUIRE(mathEdit.getMath() == "1+qwq(x)");
      REQUIRE(mathEdit.mathIsValid() == true);
      REQUIRE(mathEdit.getErrorMessage() == "");
      REQUIRE(signal.math == mathEdit.getMath());
      REQUIRE(signal.valid == mathEdit.mathIsValid());
      REQUIRE(signal.error == mathEdit.getErrorMessage());
      std::map<const std::string, std::pair<double, bool>> vars;
      // if we don't specify value for x, get nan
      REQUIRE(std::isnan(mathEdit.evaluateMath(vars)));
      // if we use the wrong interface for evaluate we get nan & logger error
      REQUIRE(std::isnan(mathEdit.evaluateMath(std::vector<double>{0})));
      // right interface for libSBML backend is map:
      vars["x"] = {4.0, false};
      REQUIRE(mathEdit.evaluateMath(vars) == dbl_approx(9.0));
    }
    GIVEN(
        "expression with user defined function (that doesn't exist in model)") {
      mathEdit.show();
      // "1+qwq(x)"
      sendKeyEvents(&mathEdit, {"1", "+", "q", "w", "q", "(", "x", ")"});
      REQUIRE(mathEdit.getMath() == "");
      REQUIRE(mathEdit.mathIsValid() == false);
      REQUIRE(mathEdit.getErrorMessage() == "Unknown function: qwq");
      REQUIRE(signal.math == mathEdit.getMath());
      REQUIRE(signal.valid == mathEdit.mathIsValid());
      REQUIRE(signal.error == mathEdit.getErrorMessage());
    }
  }
}

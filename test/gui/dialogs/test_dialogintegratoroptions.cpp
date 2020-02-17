#include <QFile>

#include "catch_wrapper.hpp"
#include "dialogintegratoroptions.hpp"
#include "qt_test_utils.hpp"
#include "simulate.hpp"

SCENARIO("DialogIntegratorOptions", "[gui][dialogs][integrator]") {
  simulate::IntegratorOptions options;
  options.order = 3;
  options.maxRelErr = 0.01;
  options.maxAbsErr = 4e-4;
  options.maxTimestep = 0.2;
  DialogIntegratorOptions dia(options);
  ModalWidgetTimer mwt;
  WHEN("user does nothing: unchanged") {
    mwt.addUserAction();
    mwt.start();
    dia.exec();
    auto opt = dia.getIntegratorOptions();
    REQUIRE(opt.order == 3);
    REQUIRE(opt.maxRelErr == dbl_approx(0.01));
    REQUIRE(opt.maxAbsErr == dbl_approx(0.0004));
    REQUIRE(opt.maxTimestep == dbl_approx(0.2));
  }
  WHEN("user changes order") {
    mwt.addUserAction({"Up, Up"});
    mwt.start();
    dia.exec();
    auto opt = dia.getIntegratorOptions();
    REQUIRE(opt.order == 2);
    REQUIRE(opt.maxRelErr == dbl_approx(0.01));
    REQUIRE(opt.maxAbsErr == dbl_approx(0.0004));
    REQUIRE(opt.maxTimestep == dbl_approx(0.2));
  }
  WHEN("user resets to defaults") {
    mwt.addUserAction({"Tab", "Tab", "Tab", "Tab", " "});
    mwt.start();
    dia.exec();
    auto opt = dia.getIntegratorOptions();
    REQUIRE(opt.order == 2);
    REQUIRE(opt.maxRelErr == dbl_approx(0.01));
    REQUIRE(opt.maxAbsErr == dbl_approx(std::numeric_limits<double>::max()));
    REQUIRE(opt.maxTimestep == dbl_approx(std::numeric_limits<double>::max()));
  }
}

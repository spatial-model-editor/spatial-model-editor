#include <QFile>

#include "catch_wrapper.hpp"
#include "dialogsimulationoptions.hpp"
#include "qt_test_utils.hpp"
#include "simulate.hpp"

SCENARIO(
    "DialogSimulationOptions",
    "[gui/dialogs/simulationoptions][gui/dialogs][gui][simulationoptions]") {
  simulate::Options options;
  options.pixel.integrator = simulate::PixelIntegratorType::RK435;
  options.pixel.maxErr = {0.01, 4e-4};
  options.pixel.maxTimestep = 0.2;
  options.dune.dt = 0.00123;
  DialogSimulationOptions dia(options);
  ModalWidgetTimer mwt;
  WHEN("user does nothing: unchanged") {
    mwt.addUserAction();
    mwt.start();
    dia.exec();
    auto opt = dia.getOptions();
    REQUIRE(opt.pixel.integrator == simulate::PixelIntegratorType::RK435);
    REQUIRE(opt.pixel.maxErr.rel == dbl_approx(4e-4));
    REQUIRE(opt.pixel.maxErr.abs == dbl_approx(0.01));
    REQUIRE(opt.pixel.maxTimestep == dbl_approx(0.2));
    REQUIRE(opt.pixel.enableMultiThreading == false);
    REQUIRE(opt.pixel.maxThreads == 0);
    REQUIRE(opt.pixel.doCSE == true);
    REQUIRE(opt.pixel.optLevel == 3);
    REQUIRE(opt.dune.dt == dbl_approx(0.00123));
    REQUIRE(opt.dune.writeVTKfiles == false);
  }
  WHEN("user changes Dune dt") {
    mwt.addUserAction({"Tab", "Tab", "9"});
    mwt.start();
    dia.exec();
    auto opt = dia.getOptions();
    REQUIRE(opt.dune.dt == dbl_approx(9));
  }
  WHEN("user changes Dune writeVTK") {
    mwt.addUserAction({"Tab", "Tab", "Tab", "Space"});
    mwt.start();
    dia.exec();
    auto opt = dia.getOptions();
    REQUIRE(opt.dune.writeVTKfiles == true);
  }
  WHEN("user changes Pixel values") {
    mwt.addUserAction({"Right", "Tab", "Up",  "Up",  "Tab",   "7",   "Tab",
                       "9",     "9",   "Tab", "0",   ".",     "5",   "Tab",
                       "Space", "Tab", "4",   "Tab", "Space", "Tab", "1"});
    mwt.start();
    dia.exec();
    auto opt = dia.getOptions();
    REQUIRE(opt.pixel.integrator == simulate::PixelIntegratorType::RK212);
    REQUIRE(opt.pixel.maxErr.rel == dbl_approx(7));
    REQUIRE(opt.pixel.maxErr.abs == dbl_approx(99));
    REQUIRE(opt.pixel.maxTimestep == dbl_approx(0.5));
    REQUIRE(opt.pixel.enableMultiThreading == true);
    REQUIRE(opt.pixel.maxThreads == 4);
    REQUIRE(opt.pixel.doCSE == false);
    REQUIRE(opt.pixel.optLevel == 1);
    REQUIRE(opt.dune.dt == dbl_approx(0.00123));
  }
  WHEN("user resets to pixel defaults") {
    mwt.addUserAction(
        {"Right", "Tab", "Tab", "Tab", "Tab", "Tab", "Tab", "Tab", "Tab", " "});
    mwt.start();
    dia.exec();
    simulate::PixelOptions defaultOpts{};
    auto opt = dia.getOptions();
    REQUIRE(opt.pixel.integrator == defaultOpts.integrator);
    REQUIRE(opt.pixel.maxErr.rel == dbl_approx(defaultOpts.maxErr.rel));
    REQUIRE(opt.pixel.maxErr.abs == dbl_approx(defaultOpts.maxErr.abs));
    REQUIRE(opt.pixel.maxTimestep == dbl_approx(defaultOpts.maxTimestep));
    REQUIRE(opt.pixel.enableMultiThreading == defaultOpts.enableMultiThreading);
    REQUIRE(opt.pixel.maxThreads == defaultOpts.maxThreads);
  }
}

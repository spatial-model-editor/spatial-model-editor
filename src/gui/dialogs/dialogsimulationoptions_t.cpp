#include "catch_wrapper.hpp"
#include "dialogsimulationoptions.hpp"
#include "qt_test_utils.hpp"
#include "simulate.hpp"
#include <QFile>

#if defined(SPATIAL_MODEL_EDITOR_WITH_TBB) ||                                  \
    defined(SPATIAL_MODEL_EDITOR_WITH_OPENMP)
SCENARIO(
    "DialogSimulationOptions",
    "[gui/dialogs/simulationoptions][gui/dialogs][gui][simulationoptions]") {
  sme::simulate::Options options;
  options.dune.integrator = "explicit_euler";
  options.dune.dt = 0.123;
  options.dune.minDt = 1e-5;
  options.dune.maxDt = 2;
  options.dune.increase = 1.11;
  options.dune.decrease = 0.77;
  options.dune.writeVTKfiles = true;
  options.dune.newtonRelErr = 4e-4;
  options.dune.newtonAbsErr = 9e-9;
  options.pixel.integrator = sme::simulate::PixelIntegratorType::RK435;
  options.pixel.maxErr = {0.01, 4e-4};
  options.pixel.maxTimestep = 0.2;
  options.pixel.enableMultiThreading = false;
  options.pixel.maxThreads = 0;
  options.pixel.doCSE = true;
  options.pixel.optLevel = 3;
  DialogSimulationOptions dia(options);
  ModalWidgetTimer mwt;
  WHEN("user does nothing: unchanged") {
    mwt.addUserAction();
    mwt.start();
    dia.exec();
    auto opt = dia.getOptions();
    REQUIRE(options.dune.integrator == "explicit_euler");
    REQUIRE(options.dune.dt == dbl_approx(0.123));
    REQUIRE(options.dune.minDt == dbl_approx(1e-5));
    REQUIRE(options.dune.maxDt == dbl_approx(2));
    REQUIRE(options.dune.increase == dbl_approx(1.11));
    REQUIRE(options.dune.decrease == dbl_approx(0.77));
    REQUIRE(options.dune.writeVTKfiles == true);
    REQUIRE(options.dune.newtonRelErr == dbl_approx(4e-4));
    REQUIRE(options.dune.newtonAbsErr == dbl_approx(9e-9));
    REQUIRE(opt.pixel.integrator == sme::simulate::PixelIntegratorType::RK435);
    REQUIRE(opt.pixel.maxErr.rel == dbl_approx(4e-4));
    REQUIRE(opt.pixel.maxErr.abs == dbl_approx(0.01));
    REQUIRE(opt.pixel.maxTimestep == dbl_approx(0.2));
    REQUIRE(opt.pixel.enableMultiThreading == false);
    REQUIRE(opt.pixel.maxThreads == 0);
    REQUIRE(opt.pixel.doCSE == true);
    REQUIRE(opt.pixel.optLevel == 3);
  }
  WHEN("user changes Dune values") {
    mwt.addUserAction({"Tab", "Tab", "Down", "Down", "9", "Tab", ".",
                       "4",   "Tab", "1",    "e",    "-", "1",   "2",
                       "Tab", "9",   "Tab",  "1",    ".", "2",   "Tab",
                       "0",   ".",   "7",    "Tab",  " ", "Tab", "1",
                       "e",   "-",   "7",    "Tab",  "0"});
    mwt.start();
    dia.exec();
    auto opt = dia.getOptions();
    REQUIRE(opt.dune.integrator == "heun");
    REQUIRE(opt.dune.dt == dbl_approx(0.4));
    REQUIRE(opt.dune.minDt == dbl_approx(1e-12));
    REQUIRE(opt.dune.maxDt == dbl_approx(9));
    REQUIRE(opt.dune.increase == 1.2);
    REQUIRE(opt.dune.decrease == 0.7);
    REQUIRE(opt.dune.writeVTKfiles == false);
    REQUIRE(opt.dune.newtonRelErr == dbl_approx(1e-7));
    REQUIRE(opt.dune.newtonAbsErr == dbl_approx(0));
  }
  WHEN("user resets to Dune defaults") {
    mwt.addUserAction({"Tab", "Tab", "Tab", "Tab", "Tab", "Tab", "Tab", "Tab",
                       "Tab", "Tab", "Tab", " "});
    mwt.start();
    dia.exec();
    sme::simulate::DuneOptions defaultOpts{};
    auto opt = dia.getOptions();
    REQUIRE(opt.dune.integrator == defaultOpts.integrator);
    REQUIRE(opt.dune.dt == dbl_approx(defaultOpts.dt));
    REQUIRE(opt.dune.minDt == dbl_approx(defaultOpts.minDt));
    REQUIRE(opt.dune.maxDt == dbl_approx(defaultOpts.maxDt));
    REQUIRE(opt.dune.increase == defaultOpts.increase);
    REQUIRE(opt.dune.decrease == defaultOpts.decrease);
    REQUIRE(opt.dune.writeVTKfiles == defaultOpts.writeVTKfiles);
  }
  WHEN("user changes Pixel values") {
    mwt.addUserAction({"Right", "Tab", "Up",  "Up",  "Tab",   "7",   "Tab",
                       "9",     "9",   "Tab", "0",   ".",     "5",   "Tab",
                       "Space", "Tab", "1",   "Tab", "Space", "Tab", "1"});
    mwt.start();
    dia.exec();
    auto opt = dia.getOptions();
    REQUIRE(opt.pixel.integrator == sme::simulate::PixelIntegratorType::RK212);
    REQUIRE(opt.pixel.maxErr.rel == dbl_approx(7));
    REQUIRE(opt.pixel.maxErr.abs == dbl_approx(99));
    REQUIRE(opt.pixel.maxTimestep == dbl_approx(0.5));
    REQUIRE(opt.pixel.enableMultiThreading == true);
    REQUIRE(opt.pixel.maxThreads == 1);
    REQUIRE(opt.pixel.doCSE == false);
    REQUIRE(opt.pixel.optLevel == 1);
  }
  WHEN("user resets to pixel defaults") {
    mwt.addUserAction(
        {"Right", "Tab", "Tab", "Tab", "Tab", "Tab", "Tab", "Tab", "Tab", " "});
    mwt.start();
    dia.exec();
    sme::simulate::PixelOptions defaultOpts{};
    auto opt = dia.getOptions();
    REQUIRE(opt.pixel.integrator == defaultOpts.integrator);
    REQUIRE(opt.pixel.maxErr.rel == dbl_approx(defaultOpts.maxErr.rel));
    REQUIRE(opt.pixel.maxErr.abs == dbl_approx(defaultOpts.maxErr.abs));
    REQUIRE(opt.pixel.maxTimestep == dbl_approx(defaultOpts.maxTimestep));
    REQUIRE(opt.pixel.enableMultiThreading == defaultOpts.enableMultiThreading);
    REQUIRE(opt.pixel.maxThreads == defaultOpts.maxThreads);
  }
}
#endif

#include "catch_wrapper.hpp"
#include "dialogsimulationoptions.hpp"
#include "qt_test_utils.hpp"
#include "sme/simulate.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>

using namespace sme::test;

struct DialogSimulationOptionsWidgets {
  explicit DialogSimulationOptionsWidgets(
      const DialogSimulationOptions *dialog) {
    GET_DIALOG_WIDGET(QTabWidget, tabSimulator);
    // DUNE tab
    GET_DIALOG_WIDGET(QComboBox, cmbDuneDiscretization);
    GET_DIALOG_WIDGET(QComboBox, cmbDuneIntegrator);
    GET_DIALOG_WIDGET(QLineEdit, txtDuneDt);
    GET_DIALOG_WIDGET(QLineEdit, txtDuneMinDt);
    GET_DIALOG_WIDGET(QLineEdit, txtDuneMaxDt);
    GET_DIALOG_WIDGET(QLineEdit, txtDuneIncrease);
    GET_DIALOG_WIDGET(QLineEdit, txtDuneDecrease);
    GET_DIALOG_WIDGET(QCheckBox, chkDuneVTK);
    GET_DIALOG_WIDGET(QLineEdit, txtDuneNewtonRel);
    GET_DIALOG_WIDGET(QLineEdit, txtDuneNewtonAbs);
    GET_DIALOG_WIDGET(QPushButton, btnDuneReset);
    // Pixel tab
    GET_DIALOG_WIDGET(QComboBox, cmbPixelIntegrator);
    GET_DIALOG_WIDGET(QLineEdit, txtPixelRelErr);
    GET_DIALOG_WIDGET(QLineEdit, txtPixelAbsErr);
    GET_DIALOG_WIDGET(QLineEdit, txtPixelDt);
    GET_DIALOG_WIDGET(QCheckBox, chkPixelMultithread);
    GET_DIALOG_WIDGET(QSpinBox, spnPixelThreads);
    GET_DIALOG_WIDGET(QCheckBox, chkPixelCSE);
    GET_DIALOG_WIDGET(QSpinBox, spnPixelOptLevel);
    GET_DIALOG_WIDGET(QPushButton, btnPixelReset);
  }
  QTabWidget *tabSimulator;
  // DUNE tab
  QComboBox *cmbDuneDiscretization;
  QComboBox *cmbDuneIntegrator;
  QLineEdit *txtDuneDt;
  QLineEdit *txtDuneMinDt;
  QLineEdit *txtDuneMaxDt;
  QLineEdit *txtDuneIncrease;
  QLineEdit *txtDuneDecrease;
  QCheckBox *chkDuneVTK;
  QLineEdit *txtDuneNewtonRel;
  QLineEdit *txtDuneNewtonAbs;
  QPushButton *btnDuneReset;
  // Pixel tab
  QComboBox *cmbPixelIntegrator;
  QLineEdit *txtPixelRelErr;
  QLineEdit *txtPixelAbsErr;
  QLineEdit *txtPixelDt;
  QCheckBox *chkPixelMultithread;
  QSpinBox *spnPixelThreads;
  QCheckBox *chkPixelCSE;
  QSpinBox *spnPixelOptLevel;
  QPushButton *btnPixelReset;
};

TEST_CASE("DialogSimulationOptions", "[gui/dialogs/simulationoptions][gui/"
                                     "dialogs][gui][simulationoptions]") {
  sme::simulate::Options options;
  options.dune.integrator = "ExplicitEuler";
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
  DialogSimulationOptionsWidgets widgets(&dia);
  dia.show();
  ModalWidgetTimer mwt;
  SECTION("user does nothing: unchanged") {
    auto opt = dia.getOptions();
    REQUIRE(options.dune.integrator == "ExplicitEuler");
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
  SECTION("user changes Dune values, then resets to defaults") {
    widgets.tabSimulator->setCurrentIndex(0);
    widgets.cmbDuneIntegrator->setCurrentIndex(2);
    widgets.txtDuneDt->clear();
    sendKeyEvents(widgets.txtDuneDt, {"0", ".", "4", "Enter"});
    widgets.txtDuneMinDt->clear();
    sendKeyEvents(widgets.txtDuneMinDt, {"1", "e", "-", "1", "2", "Enter"});
    widgets.txtDuneMaxDt->clear();
    sendKeyEvents(widgets.txtDuneMaxDt, {"9", "Enter"});
    widgets.txtDuneIncrease->clear();
    sendKeyEvents(widgets.txtDuneIncrease, {"1", ".", "2", "Enter"});
    widgets.txtDuneDecrease->clear();
    sendKeyEvents(widgets.txtDuneDecrease, {"0", ".", "7", "Enter"});
    widgets.chkDuneVTK->setChecked(false);
    widgets.txtDuneNewtonRel->clear();
    sendKeyEvents(widgets.txtDuneNewtonRel, {"1", "e", "-", "7", "Enter"});
    widgets.txtDuneNewtonAbs->clear();
    sendKeyEvents(widgets.txtDuneNewtonAbs, {"0", "Enter"});
    auto opt = dia.getOptions();
    REQUIRE(opt.dune.integrator == "Heun");
    REQUIRE(opt.dune.dt == dbl_approx(0.4));
    REQUIRE(opt.dune.minDt == dbl_approx(1e-12));
    REQUIRE(opt.dune.maxDt == dbl_approx(9));
    REQUIRE(opt.dune.increase == 1.2);
    REQUIRE(opt.dune.decrease == 0.7);
    REQUIRE(opt.dune.writeVTKfiles == false);
    REQUIRE(opt.dune.newtonRelErr == dbl_approx(1e-7));
    REQUIRE(opt.dune.newtonAbsErr == dbl_approx(0));
    sendMouseClick(widgets.btnDuneReset);
    sme::simulate::DuneOptions defaultOpts{};
    opt = dia.getOptions();
    REQUIRE(opt.dune.integrator == defaultOpts.integrator);
    REQUIRE(opt.dune.dt == dbl_approx(defaultOpts.dt));
    REQUIRE(opt.dune.minDt == dbl_approx(defaultOpts.minDt));
    REQUIRE(opt.dune.maxDt == dbl_approx(defaultOpts.maxDt));
    REQUIRE(opt.dune.increase == defaultOpts.increase);
    REQUIRE(opt.dune.decrease == defaultOpts.decrease);
    REQUIRE(opt.dune.writeVTKfiles == defaultOpts.writeVTKfiles);
  }
  SECTION("user changes Pixel values, then resets to defaults") {
    widgets.tabSimulator->setCurrentIndex(0);
    widgets.cmbPixelIntegrator->setCurrentIndex(2);
    widgets.txtPixelRelErr->clear();
    sendKeyEvents(widgets.txtPixelRelErr, {"0", ".", "4", "Enter"});
    widgets.txtPixelAbsErr->clear();
    sendKeyEvents(widgets.txtPixelAbsErr, {"2", ".", "7", "Enter"});
    widgets.txtPixelDt->clear();
    sendKeyEvents(widgets.txtPixelDt, {"0", ".", "0", "1", "8", "Enter"});
    widgets.chkPixelMultithread->setChecked(true);
    widgets.spnPixelThreads->setValue(1);
    widgets.chkPixelCSE->setChecked(false);
    widgets.spnPixelOptLevel->setValue(1);
    auto opt = dia.getOptions();
    REQUIRE(opt.pixel.integrator == sme::simulate::PixelIntegratorType::RK323);
    REQUIRE(opt.pixel.maxErr.rel == dbl_approx(0.4));
    REQUIRE(opt.pixel.maxErr.abs == dbl_approx(2.7));
    REQUIRE(opt.pixel.maxTimestep == dbl_approx(0.018));
    REQUIRE(opt.pixel.enableMultiThreading == true);
    REQUIRE(opt.pixel.maxThreads == 1);
    REQUIRE(opt.pixel.doCSE == false);
    REQUIRE(opt.pixel.optLevel == 1);
    sendMouseClick(widgets.btnPixelReset);
    sme::simulate::PixelOptions defaultOpts{};
    opt = dia.getOptions();
    REQUIRE(opt.pixel.integrator == defaultOpts.integrator);
    REQUIRE(opt.pixel.maxErr.rel == dbl_approx(defaultOpts.maxErr.rel));
    REQUIRE(opt.pixel.maxErr.abs == dbl_approx(defaultOpts.maxErr.abs));
    REQUIRE(opt.pixel.maxTimestep == dbl_approx(defaultOpts.maxTimestep));
    REQUIRE(opt.pixel.enableMultiThreading == defaultOpts.enableMultiThreading);
    REQUIRE(opt.pixel.maxThreads == defaultOpts.maxThreads);
    REQUIRE(opt.pixel.doCSE == defaultOpts.doCSE);
    REQUIRE(opt.pixel.optLevel == defaultOpts.optLevel);
  }
}

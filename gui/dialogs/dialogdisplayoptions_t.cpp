#include "catch_wrapper.hpp"
#include "dialogdisplayoptions.hpp"
#include "plotwrapper.hpp"
#include "qt_test_utils.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QStringList>
#include <QTreeWidget>

using namespace sme::test;

struct DialogDisplayOptionsWidgets {
  explicit DialogDisplayOptionsWidgets(const DialogDisplayOptions *dialog) {
    GET_DIALOG_WIDGET(QTreeWidget, listSpecies);
    GET_DIALOG_WIDGET(QCheckBox, chkShowMinMaxRanges);
    GET_DIALOG_WIDGET(QTreeWidget, listObservables);
    GET_DIALOG_WIDGET(QPushButton, btnAddObservable);
    GET_DIALOG_WIDGET(QPushButton, btnEditObservable);
    GET_DIALOG_WIDGET(QPushButton, btnRemoveObservable);
    GET_DIALOG_WIDGET(QComboBox, cmbNormaliseOverAllSpecies);
    GET_DIALOG_WIDGET(QComboBox, cmbNormaliseOverAllTimepoints);
  }
  QTreeWidget *listSpecies;
  QCheckBox *chkShowMinMaxRanges;
  QTreeWidget *listObservables;
  QPushButton *btnAddObservable;
  QPushButton *btnEditObservable;
  QPushButton *btnRemoveObservable;
  QComboBox *cmbNormaliseOverAllSpecies;
  QComboBox *cmbNormaliseOverAllTimepoints;
};

TEST_CASE("DialogDisplayOptions",
          "[gui/dialogs/displayoptions][gui/dialogs][gui][displayoptions]") {
  SECTION("Existing options, no observables") {
    QStringList compartments{"c1", "c2"};
    std::vector<QStringList> species;
    species.push_back({"s1_c1", "s2_c1"});
    species.push_back({"s1_c2", "s2_c2", "s3_c2"});
    sme::model::DisplayOptions opts;
    opts.showSpecies = {true, false, false, true, false};
    opts.showMinMax = false;
    opts.normaliseOverAllTimepoints = false;
    opts.normaliseOverAllSpecies = false;
    DialogDisplayOptions dia(compartments, species, opts, {});
    dia.show();
    DialogDisplayOptionsWidgets widgets(&dia);
    SECTION("user does nothing") {
      REQUIRE(dia.getShowSpecies()[0] == true);
      REQUIRE(dia.getShowSpecies()[1] == false);
      REQUIRE(dia.getShowMinMax() == false);
      REQUIRE(dia.getNormaliseOverAllTimepoints() == false);
      REQUIRE(dia.getNormaliseOverAllSpecies() == false);
    }
    SECTION(
        "user checks first compartment: enables all species in compartment") {
      REQUIRE(dia.getShowSpecies()[0] == true);
      REQUIRE(dia.getShowSpecies()[1] == false);
      widgets.listSpecies->topLevelItem(0)->setCheckState(
          0, Qt::CheckState::Checked);
      REQUIRE(dia.getShowSpecies()[0] == true);
      REQUIRE(dia.getShowSpecies()[1] == true);
      REQUIRE(dia.getNormaliseOverAllTimepoints() == false);
      REQUIRE(dia.getNormaliseOverAllSpecies() == false);
    }
    SECTION(
        "user checks, then unchecks first compartment: disables all species in "
        "compartment") {
      REQUIRE(dia.getShowSpecies()[0] == true);
      REQUIRE(dia.getShowSpecies()[1] == false);
      widgets.listSpecies->topLevelItem(0)->setCheckState(
          0, Qt::CheckState::Checked);
      widgets.listSpecies->topLevelItem(0)->setCheckState(
          0, Qt::CheckState::Unchecked);
      REQUIRE(dia.getShowSpecies()[0] == false);
      REQUIRE(dia.getShowSpecies()[1] == false);
      REQUIRE(dia.getNormaliseOverAllTimepoints() == false);
      REQUIRE(dia.getNormaliseOverAllSpecies() == false);
    }
    SECTION("user unchecks first species") {
      REQUIRE(dia.getShowSpecies()[0] == true);
      REQUIRE(dia.getShowSpecies()[1] == false);
      widgets.listSpecies->topLevelItem(0)->child(0)->setCheckState(
          0, Qt::CheckState::Unchecked);
      REQUIRE(dia.getShowSpecies()[0] == false);
      REQUIRE(dia.getShowSpecies()[1] == false);
      REQUIRE(dia.getNormaliseOverAllTimepoints() == false);
      REQUIRE(dia.getNormaliseOverAllSpecies() == false);
    }
    SECTION("user unchecks then checks first species") {
      REQUIRE(dia.getShowSpecies()[0] == true);
      REQUIRE(dia.getShowSpecies()[1] == false);
      widgets.listSpecies->topLevelItem(0)->child(0)->setCheckState(
          0, Qt::CheckState::Unchecked);
      widgets.listSpecies->topLevelItem(0)->child(0)->setCheckState(
          0, Qt::CheckState::Checked);
      REQUIRE(dia.getShowSpecies()[0] == true);
      REQUIRE(dia.getShowSpecies()[1] == false);
      REQUIRE(dia.getNormaliseOverAllTimepoints() == false);
      REQUIRE(dia.getNormaliseOverAllSpecies() == false);
    }
    SECTION("user toggles show min/max checkbox") {
      REQUIRE(dia.getShowMinMax() == false);
      sendMouseClick(widgets.chkShowMinMaxRanges);
      REQUIRE(dia.getShowMinMax() == true);
      REQUIRE(dia.getNormaliseOverAllTimepoints() == false);
      REQUIRE(dia.getNormaliseOverAllSpecies() == false);
    }
    SECTION("user toggles show min/max checkbox twice") {
      REQUIRE(dia.getShowMinMax() == false);
      sendMouseClick(widgets.chkShowMinMaxRanges);
      REQUIRE(dia.getShowMinMax() == true);
      sendMouseClick(widgets.chkShowMinMaxRanges);
      REQUIRE(dia.getShowMinMax() == false);
      REQUIRE(dia.getNormaliseOverAllTimepoints() == false);
      REQUIRE(dia.getNormaliseOverAllSpecies() == false);
    }
    SECTION("user changes timepoint normalisation") {
      REQUIRE(dia.getNormaliseOverAllTimepoints() == false);
      REQUIRE(dia.getNormaliseOverAllSpecies() == false);
      widgets.cmbNormaliseOverAllTimepoints->setCurrentIndex(1);
      REQUIRE(dia.getNormaliseOverAllTimepoints() == true);
      REQUIRE(dia.getNormaliseOverAllSpecies() == false);
    }
    SECTION("user changes species normalisation") {
      REQUIRE(dia.getNormaliseOverAllTimepoints() == false);
      REQUIRE(dia.getNormaliseOverAllSpecies() == false);
      widgets.cmbNormaliseOverAllSpecies->setCurrentIndex(1);
      REQUIRE(dia.getNormaliseOverAllTimepoints() == false);
      REQUIRE(dia.getNormaliseOverAllSpecies() == true);
    }
    SECTION("user changes both normalisations") {
      REQUIRE(dia.getNormaliseOverAllTimepoints() == false);
      REQUIRE(dia.getNormaliseOverAllSpecies() == false);
      widgets.cmbNormaliseOverAllTimepoints->setCurrentIndex(1);
      widgets.cmbNormaliseOverAllSpecies->setCurrentIndex(1);
      REQUIRE(dia.getNormaliseOverAllTimepoints() == true);
      REQUIRE(dia.getNormaliseOverAllSpecies() == true);
    }
    SECTION("user checks both compartment: enables all species") {
      REQUIRE(dia.getShowSpecies() == opts.showSpecies);
      widgets.listSpecies->topLevelItem(0)->setCheckState(
          0, Qt::CheckState::Checked);
      widgets.listSpecies->topLevelItem(1)->setCheckState(
          0, Qt::CheckState::Checked);
      REQUIRE(dia.getShowSpecies().size() == 5);
      REQUIRE(dia.getShowSpecies()[0] == true);
      REQUIRE(dia.getShowSpecies()[1] == true);
      REQUIRE(dia.getShowSpecies()[2] == true);
      REQUIRE(dia.getShowSpecies()[3] == true);
      REQUIRE(dia.getShowSpecies()[4] == true);
    }
    SECTION("user checks & unchecks both compartments: disable all species") {
      widgets.listSpecies->topLevelItem(0)->setCheckState(
          0, Qt::CheckState::Checked);
      widgets.listSpecies->topLevelItem(1)->setCheckState(
          0, Qt::CheckState::Checked);
      widgets.listSpecies->topLevelItem(0)->setCheckState(
          0, Qt::CheckState::Unchecked);
      widgets.listSpecies->topLevelItem(1)->setCheckState(
          0, Qt::CheckState::Unchecked);
      REQUIRE(dia.getShowSpecies().size() == 5);
      REQUIRE(dia.getShowSpecies()[0] == false);
      REQUIRE(dia.getShowSpecies()[1] == false);
      REQUIRE(dia.getShowSpecies()[2] == false);
      REQUIRE(dia.getShowSpecies()[3] == false);
      REQUIRE(dia.getShowSpecies()[4] == false);
      REQUIRE(dia.getNormaliseOverAllTimepoints() == false);
      REQUIRE(dia.getNormaliseOverAllSpecies() == false);
    }
  }
  SECTION("Existing options, existing observables") {
    QStringList compartments{"c1", "c2"};
    std::vector<QStringList> species;
    species.push_back({"s1_c1", "s2_c1"});
    species.push_back({"s1_c2", "s2_c2", "s3_c2"});
    sme::model::DisplayOptions opts;
    opts.showSpecies = {true, false, false, true, false};
    opts.showMinMax = false;
    opts.normaliseOverAllTimepoints = false;
    opts.normaliseOverAllSpecies = false;
    std::vector<PlotWrapperObservable> observables;
    observables.push_back({"1", "1", true});
    observables.push_back({"s1_c1*2", "s1_c1*2", true});
    DialogDisplayOptions dia(compartments, species, opts, observables);
    dia.show();
    DialogDisplayOptionsWidgets widgets(&dia);
    SECTION("user does nothing") {
      REQUIRE(dia.getShowSpecies() == opts.showSpecies);
      REQUIRE(dia.getShowMinMax() == false);
      REQUIRE(dia.getNormaliseOverAllTimepoints() == false);
      REQUIRE(dia.getNormaliseOverAllSpecies() == false);
      REQUIRE(dia.getObservables().size() == 2);
      REQUIRE(dia.getObservables()[0].visible == true);
      REQUIRE(dia.getObservables()[1].visible == true);
    }
    SECTION("user hides first observable") {
      widgets.listObservables->topLevelItem(0)->setCheckState(
          0, Qt::CheckState::Unchecked);
      REQUIRE(dia.getObservables().size() == 2);
      REQUIRE(dia.getObservables()[0].visible == false);
      REQUIRE(dia.getObservables()[1].visible == true);
    }
    SECTION("user hides both observables") {
      widgets.listObservables->topLevelItem(0)->setCheckState(
          0, Qt::CheckState::Unchecked);
      widgets.listObservables->topLevelItem(1)->setCheckState(
          0, Qt::CheckState::Unchecked);
      REQUIRE(dia.getObservables().size() == 2);
      REQUIRE(dia.getObservables()[0].visible == false);
      REQUIRE(dia.getObservables()[1].visible == false);
    }
    SECTION("user adds new observable") {
      ModalWidgetTimer mwt;
      mwt.addUserAction({"1", "+", "2"});
      mwt.setIgnoredWidget(&dia);
      mwt.start();
      sendMouseClick(widgets.btnAddObservable);
      REQUIRE(dia.getObservables().size() == 3);
      REQUIRE(dia.getObservables()[2].name == "1+2");
      REQUIRE(dia.getObservables()[2].expression == "1+2");
      REQUIRE(dia.getObservables()[2].visible == true);
    }
    SECTION("user edits first observable") {
      ModalWidgetTimer mwt;
      mwt.addUserAction({"Right", "-", "2"});
      mwt.setIgnoredWidget(&dia);
      mwt.start();
      sendMouseClick(widgets.listObservables->topLevelItem(0));
      sendMouseClick(widgets.btnEditObservable);
      REQUIRE(dia.getObservables().size() == 2);
      REQUIRE(dia.getObservables()[0].name == "1-2");
      REQUIRE(dia.getObservables()[0].expression == "1-2");
      REQUIRE(dia.getObservables()[0].visible == true);
      REQUIRE(dia.getObservables()[1].name == "s1_c1*2");
      REQUIRE(dia.getObservables()[1].expression == "s1_c1*2");
      REQUIRE(dia.getObservables()[1].visible == true);
    }
    SECTION("user removes first observable") {
      sendMouseClick(widgets.listObservables->topLevelItem(0));
      sendMouseClick(widgets.btnRemoveObservable);
      REQUIRE(dia.getObservables().size() == 1);
      REQUIRE(dia.getObservables()[0].name == "s1_c1*2");
      REQUIRE(dia.getObservables()[0].expression == "s1_c1*2");
      REQUIRE(dia.getObservables()[0].visible == true);
    }
  }
}

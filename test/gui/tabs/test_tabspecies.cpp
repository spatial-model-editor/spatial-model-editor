#include <QCheckBox>
#include <QComboBox>
#include <QFile>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QTreeWidget>

#include "catch_wrapper.hpp"
#include "qlabelmousetracker.hpp"
#include "qt_test_utils.hpp"
#include "sbml.hpp"
#include "tabspecies.hpp"

SCENARIO("Species Tab", "[gui][tabs][species]") {
  sbml::SbmlDocWrapper sbmlDoc;
  QLabelMouseTracker mouseTracker;
  auto tab = TabSpecies(sbmlDoc, &mouseTracker);
  tab.show();
  waitFor(&tab);

  ModalWidgetTimer mwt;
  // get pointers to widgets within tab
  auto *listSpecies = tab.findChild<QTreeWidget *>("listSpecies");
  auto *btnAddSpecies = tab.findChild<QPushButton *>("btnAddSpecies");
  auto *btnRemoveSpecies = tab.findChild<QPushButton *>("btnRemoveSpecies");
  auto *txtSpeciesName = tab.findChild<QLineEdit *>("txtSpeciesName");
  auto *cmbSpeciesCompartment =
      tab.findChild<QComboBox *>("cmbSpeciesCompartment");
  auto *chkSpeciesIsSpatial = tab.findChild<QCheckBox *>("chkSpeciesIsSpatial");
  auto *chkSpeciesIsConstant =
      tab.findChild<QCheckBox *>("chkSpeciesIsConstant");
  auto *radInitialConcentrationUniform =
      tab.findChild<QRadioButton *>("radInitialConcentrationUniform");
  auto *txtInitialConcentration =
      tab.findChild<QLineEdit *>("txtInitialConcentration");
  auto *radInitialConcentrationAnalytic =
      tab.findChild<QRadioButton *>("radInitialConcentrationAnalytic");
  auto *btnEditAnalyticConcentration =
      tab.findChild<QPushButton *>("btnEditAnalyticConcentration");
  auto *radInitialConcentrationImage =
      tab.findChild<QRadioButton *>("radInitialConcentrationImage");
  auto *btnEditImageConcentration =
      tab.findChild<QPushButton *>("btnEditImageConcentration");
  auto *txtDiffusionConstant =
      tab.findChild<QLineEdit *>("txtDiffusionConstant");
  auto *btnChangeSpeciesColour =
      tab.findChild<QPushButton *>("btnChangeSpeciesColour");

  WHEN("very-simple-model loaded") {
    if (QFile f(":/models/very-simple-model.xml");
        f.open(QIODevice::ReadOnly)) {
      sbmlDoc.importSBMLString(f.readAll().toStdString());
    }
    tab.loadModelData();

    // initial selection: Outside / A [constant species]

    REQUIRE(listSpecies->currentItem()->text(0) == "A_out");
    REQUIRE(listSpecies->currentItem()->parent()->text(0) == "Outside");
    REQUIRE(chkSpeciesIsConstant->isChecked() == true);
    REQUIRE(chkSpeciesIsSpatial->isChecked() == false);
    REQUIRE(radInitialConcentrationUniform->isEnabled() == true);
    REQUIRE(radInitialConcentrationAnalytic->isEnabled() == false);
    REQUIRE(radInitialConcentrationImage->isEnabled() == false);
    REQUIRE(txtDiffusionConstant->isEnabled() == false);
    // edit name
    txtSpeciesName->setFocus();
    sendKeyEvents(txtSpeciesName, {" ", "!", "Enter"});
    REQUIRE(listSpecies->currentItem()->text(0) == "A_out !");
    sendKeyEvents(txtSpeciesName, {"End", "Backspace", "Backspace", "Enter"});
    REQUIRE(listSpecies->currentItem()->text(0) == "A_out");
    // toggle is spatial checkbox
    sendMouseClick(chkSpeciesIsSpatial);
    REQUIRE(chkSpeciesIsSpatial->isChecked() == true);
    REQUIRE(chkSpeciesIsConstant->isChecked() == false);
    REQUIRE(radInitialConcentrationUniform->isEnabled() == true);
    REQUIRE(radInitialConcentrationAnalytic->isEnabled() == true);
    REQUIRE(radInitialConcentrationImage->isEnabled() == true);
    REQUIRE(txtDiffusionConstant->isEnabled() == true);
    sendMouseClick(chkSpeciesIsSpatial);
    REQUIRE(chkSpeciesIsSpatial->isChecked() == false);
    REQUIRE(chkSpeciesIsConstant->isChecked() == false);
    REQUIRE(radInitialConcentrationUniform->isEnabled() == true);
    REQUIRE(radInitialConcentrationAnalytic->isEnabled() == false);
    REQUIRE(radInitialConcentrationImage->isEnabled() == false);
    REQUIRE(txtDiffusionConstant->isEnabled() == false);
    // toggle is constant checkbox
    sendMouseClick(chkSpeciesIsConstant);
    REQUIRE(chkSpeciesIsConstant->isChecked() == true);
    REQUIRE(chkSpeciesIsSpatial->isChecked() == false);
    sendMouseClick(chkSpeciesIsConstant);
    REQUIRE(chkSpeciesIsConstant->isChecked() == false);
    REQUIRE(chkSpeciesIsSpatial->isChecked() == false);

    // select second item in listSpecies: Outside / B [spatial species]

    sendKeyEvents(listSpecies, {"Down"});
    REQUIRE(listSpecies->currentItem()->text(0) == "B_out");
    REQUIRE(listSpecies->currentItem()->parent()->text(0) == "Outside");
    REQUIRE(chkSpeciesIsConstant->isChecked() == false);
    REQUIRE(chkSpeciesIsSpatial->isChecked() == true);
    // edit initial concentration
    REQUIRE(radInitialConcentrationUniform->isChecked());
    txtInitialConcentration->setFocus();
    REQUIRE(txtInitialConcentration->text() != "2");
    sendKeyEvents(txtInitialConcentration, {"End", "Backspace", "2", "Enter"});
    sendKeyEvents(listSpecies, {"Up"});
    sendKeyEvents(listSpecies, {"Down"});
    REQUIRE(txtInitialConcentration->text() == "2");
    // enable and edit analytic expression
    REQUIRE(btnEditAnalyticConcentration->isEnabled() == false);
    sendMouseClick(radInitialConcentrationAnalytic);
    REQUIRE(btnEditAnalyticConcentration->isEnabled() == true);
    mwt.addUserAction({"1", "+", "2"});
    mwt.start();
    sendMouseClick(btnEditAnalyticConcentration);
    // enable and edit image initial concentration
    REQUIRE(btnEditImageConcentration->isEnabled() == false);
    sendMouseClick(radInitialConcentrationImage);
    REQUIRE(btnEditImageConcentration->isEnabled() == true);
    mwt.addUserAction({"Esc"});
    mwt.start();
    sendMouseClick(btnEditImageConcentration);
    sendMouseClick(radInitialConcentrationUniform);
    // edit diffusion constant
    txtDiffusionConstant->setFocus();
    REQUIRE(txtDiffusionConstant->text() != "9");
    sendKeyEvents(txtDiffusionConstant, {"End", "Backspace", "9", "Enter"});
    sendKeyEvents(listSpecies, {"Up"});
    sendKeyEvents(listSpecies, {"Down"});
    REQUIRE(txtDiffusionConstant->text() == "9");
    // click change species colour, then cancel
    mwt.addUserAction({"Esc"});
    mwt.start();
    sendMouseClick(btnChangeSpeciesColour);
    // edit name
    txtSpeciesName->setFocus();
    sendKeyEvents(txtSpeciesName, {"B", "Q", "Tab"});
    REQUIRE(listSpecies->currentItem()->text(0) == "B_outBQ");
    REQUIRE(listSpecies->currentItem()->parent()->text(0) == "Outside");
    // change species location
    cmbSpeciesCompartment->setFocus();
    sendKeyEvents(cmbSpeciesCompartment, {"Down"});
    REQUIRE(listSpecies->currentItem()->text(0) == "B_outBQ");
    REQUIRE(listSpecies->currentItem()->parent()->text(0) == "Cell");
    cmbSpeciesCompartment->setFocus();
    sendKeyEvents(cmbSpeciesCompartment, {"Down"});
    REQUIRE(listSpecies->currentItem()->text(0) == "B_outBQ");
    REQUIRE(listSpecies->currentItem()->parent()->text(0) == "Nucleus");
    cmbSpeciesCompartment->setFocus();
    sendKeyEvents(cmbSpeciesCompartment, {"Up"});
    REQUIRE(listSpecies->currentItem()->text(0) == "B_outBQ");
    REQUIRE(listSpecies->currentItem()->parent()->text(0) == "Cell");
    cmbSpeciesCompartment->setFocus();
    sendKeyEvents(cmbSpeciesCompartment, {"Up"});
    REQUIRE(listSpecies->currentItem()->text(0) == "B_outBQ");
    REQUIRE(listSpecies->currentItem()->parent()->text(0) == "Outside");
    // edit name
    sendKeyEvents(txtSpeciesName, {"End", "Backspace", "Backspace", "Enter"});
    REQUIRE(listSpecies->currentItem()->text(0) == "B_out");

    // toggle is spatial checkbox
    sendMouseClick(chkSpeciesIsSpatial);
    REQUIRE(chkSpeciesIsSpatial->isChecked() == false);
    REQUIRE(chkSpeciesIsConstant->isChecked() == false);
    sendMouseClick(chkSpeciesIsSpatial);
    REQUIRE(chkSpeciesIsSpatial->isChecked() == true);

    // add/remove species

    sendKeyEvents(listSpecies, {"Up"});
    REQUIRE(listSpecies->currentItem()->text(0) == "A_out");
    // click remove species, then "no" to cancel
    sendMouseClick(btnRemoveSpecies);
    sendKeyEventsToNextQDialog({"Esc"});
    REQUIRE(listSpecies->currentItem()->text(0) == "A_out");
    REQUIRE(listSpecies->currentItem()->parent()->text(0) == "Outside");
    // click remove species, then "yes" to confirm
    sendMouseClick(btnRemoveSpecies);
    sendKeyEventsToNextQDialog({"Enter"});
    REQUIRE(listSpecies->currentItem()->text(0) == "B_out");
    REQUIRE(listSpecies->currentItem()->parent()->text(0) == "Outside");
    // click remove species, then "yes" to confirm
    sendMouseClick(btnRemoveSpecies);
    sendKeyEventsToNextQDialog({"Enter"});
    REQUIRE(listSpecies->currentItem()->text(0) == "A_cell");
    REQUIRE(listSpecies->currentItem()->parent()->text(0) == "Cell");
    // click remove species, then "yes" to confirm
    sendMouseClick(btnRemoveSpecies);
    sendKeyEventsToNextQDialog({"Enter"});
    REQUIRE(listSpecies->currentItem()->text(0) == "B_cell");
    REQUIRE(listSpecies->currentItem()->parent()->text(0) == "Cell");
    // click remove species, then "yes" to confirm
    sendMouseClick(btnRemoveSpecies);
    sendKeyEventsToNextQDialog({"Enter"});
    REQUIRE(listSpecies->currentItem()->text(0) == "A_nucl");
    REQUIRE(listSpecies->currentItem()->parent()->text(0) == "Nucleus");
    REQUIRE(listSpecies->currentItem()->parent()->childCount() == 2);
    // click add species
    mwt.addUserAction({"n", "e", "w", " ", "s", "p", "e", "c", "i", "e", "s",
                       " ", "i", "n", " ", "n", "u", "c", "l", "e", "u", "s"});
    mwt.start();
    sendMouseClick(btnAddSpecies);
    REQUIRE(listSpecies->currentItem()->text(0) == "new species in nucleus");
    REQUIRE(listSpecies->currentItem()->parent()->text(0) == "Nucleus");
    REQUIRE(listSpecies->currentItem()->parent()->childCount() == 3);
    // click remove species, then "no" to cancel
    sendMouseClick(btnRemoveSpecies);
    sendKeyEventsToNextQDialog({"Esc"});
    REQUIRE(listSpecies->currentItem()->text(0) == "new species in nucleus");
    REQUIRE(listSpecies->currentItem()->parent()->text(0) == "Nucleus");
    REQUIRE(listSpecies->currentItem()->parent()->childCount() == 3);
    // click remove species, then "yes" to confirm
    sendMouseClick(btnRemoveSpecies);
    sendKeyEventsToNextQDialog({"Enter"});
    REQUIRE(listSpecies->currentItem()->text(0) == "A_nucl");
    REQUIRE(listSpecies->currentItem()->parent()->text(0) == "Nucleus");
    REQUIRE(listSpecies->currentItem()->parent()->childCount() == 2);
    // click remove species, then "yes" to confirm
    sendMouseClick(btnRemoveSpecies);
    sendKeyEventsToNextQDialog({"Enter"});
    REQUIRE(listSpecies->currentItem()->text(0) == "B_nucl");
    REQUIRE(listSpecies->currentItem()->parent()->text(0) == "Nucleus");
    REQUIRE(listSpecies->currentItem()->parent()->childCount() == 1);
    // click remove species, then "yes" to confirm
    sendMouseClick(btnRemoveSpecies);
    sendKeyEventsToNextQDialog({"Enter"});
    // no species left in model:
    REQUIRE(listSpecies->topLevelItem(0)->childCount() == 0);
    REQUIRE(listSpecies->topLevelItem(1)->childCount() == 0);
    REQUIRE(listSpecies->topLevelItem(2)->childCount() == 0);
    // add species - no current selection so defaults to first compartment
    // click add species
    mwt.addUserAction({"n", "e", "w", " ", "s", "p", "e", "c", "!"});
    mwt.start();
    sendMouseClick(btnAddSpecies);
    REQUIRE(listSpecies->currentItem()->text(0) == "new spec!");
    REQUIRE(listSpecies->currentItem()->parent()->text(0) == "Outside");
    REQUIRE(listSpecies->currentItem()->parent()->childCount() == 1);
  }
}

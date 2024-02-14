#include "catch_wrapper.hpp"
#include "model_test_utils.hpp"
#include "qlabelmousetracker.hpp"
#include "qt_test_utils.hpp"
#include "qvoxelrenderer.hpp"
#include "sme/model.hpp"
#include "tabspecies.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QTreeWidget>

using namespace sme::test;

TEST_CASE("TabSpecies", "[gui/tabs/species][gui/tabs][gui][species]") {
  sme::model::Model model;
  QLabelMouseTracker mouseTracker;
  QVoxelRenderer voxelRenderer;
  TabSpecies tab(model, &mouseTracker, &voxelRenderer);
  tab.show();
  waitFor(&tab);

  ModalWidgetTimer mwt;
  // get pointers to widgets within tab
  auto *listSpecies{tab.findChild<QTreeWidget *>("listSpecies")};
  REQUIRE(listSpecies != nullptr);
  auto *btnAddSpecies{tab.findChild<QPushButton *>("btnAddSpecies")};
  REQUIRE(btnAddSpecies != nullptr);
  auto *btnRemoveSpecies{tab.findChild<QPushButton *>("btnRemoveSpecies")};
  REQUIRE(btnRemoveSpecies != nullptr);
  auto *txtSpeciesName{tab.findChild<QLineEdit *>("txtSpeciesName")};
  REQUIRE(txtSpeciesName != nullptr);
  auto *cmbSpeciesCompartment{
      tab.findChild<QComboBox *>("cmbSpeciesCompartment")};
  REQUIRE(cmbSpeciesCompartment != nullptr);
  auto *chkSpeciesIsSpatial{tab.findChild<QCheckBox *>("chkSpeciesIsSpatial")};
  REQUIRE(chkSpeciesIsSpatial != nullptr);
  auto *chkSpeciesIsConstant{
      tab.findChild<QCheckBox *>("chkSpeciesIsConstant")};
  REQUIRE(chkSpeciesIsConstant != nullptr);
  auto *radInitialConcentrationUniform{
      tab.findChild<QRadioButton *>("radInitialConcentrationUniform")};
  REQUIRE(radInitialConcentrationUniform != nullptr);
  auto *txtInitialConcentration{
      tab.findChild<QLineEdit *>("txtInitialConcentration")};
  REQUIRE(txtInitialConcentration != nullptr);
  auto *radInitialConcentrationAnalytic{
      tab.findChild<QRadioButton *>("radInitialConcentrationAnalytic")};
  REQUIRE(radInitialConcentrationAnalytic != nullptr);
  auto *btnEditAnalyticConcentration{
      tab.findChild<QPushButton *>("btnEditAnalyticConcentration")};
  REQUIRE(btnEditAnalyticConcentration != nullptr);
  auto *radInitialConcentrationImage{
      tab.findChild<QRadioButton *>("radInitialConcentrationImage")};
  REQUIRE(radInitialConcentrationImage != nullptr);
  auto *btnEditImageConcentration{
      tab.findChild<QPushButton *>("btnEditImageConcentration")};
  REQUIRE(btnEditImageConcentration != nullptr);
  auto *txtDiffusionConstant{
      tab.findChild<QLineEdit *>("txtDiffusionConstant")};
  REQUIRE(txtDiffusionConstant != nullptr);
  auto *btnChangeSpeciesColour{
      tab.findChild<QPushButton *>("btnChangeSpeciesColour")};
  REQUIRE(btnChangeSpeciesColour != nullptr);

  SECTION("very-simple-model loaded") {
    model = getExampleModel(Mod::VerySimpleModel);
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
    sendKeyEvents(chkSpeciesIsSpatial, {" "});
    REQUIRE(chkSpeciesIsSpatial->isChecked() == true);
    REQUIRE(chkSpeciesIsConstant->isChecked() == false);
    REQUIRE(radInitialConcentrationUniform->isEnabled() == true);
    REQUIRE(radInitialConcentrationAnalytic->isEnabled() == true);
    REQUIRE(radInitialConcentrationImage->isEnabled() == true);
    REQUIRE(txtDiffusionConstant->isEnabled() == true);
    sendKeyEvents(chkSpeciesIsSpatial, {" "});
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
    sendKeyEvents(chkSpeciesIsSpatial, {" "});
    REQUIRE(chkSpeciesIsSpatial->isChecked() == false);
    REQUIRE(chkSpeciesIsConstant->isChecked() == false);
    sendKeyEvents(chkSpeciesIsSpatial, {" "});
    REQUIRE(chkSpeciesIsSpatial->isChecked() == true);

    // add/remove species

    sendKeyEvents(listSpecies, {"Up"});
    REQUIRE(listSpecies->currentItem()->text(0) == "A_out");
    // click remove species, then "no" to cancel
    mwt.addUserAction({"Esc"});
    mwt.start();
    sendMouseClick(btnRemoveSpecies);
    REQUIRE(listSpecies->currentItem()->text(0) == "A_out");
    REQUIRE(listSpecies->currentItem()->parent()->text(0) == "Outside");
    // click remove species, then "yes" to confirm
    mwt.addUserAction({"Enter"});
    mwt.start();
    sendMouseClick(btnRemoveSpecies);
    REQUIRE(listSpecies->currentItem()->text(0) == "B_out");
    REQUIRE(listSpecies->currentItem()->parent()->text(0) == "Outside");
    // click remove species, then "yes" to confirm
    mwt.addUserAction({"Enter"});
    mwt.start();
    sendMouseClick(btnRemoveSpecies);
    REQUIRE(listSpecies->currentItem()->text(0) == "A_cell");
    REQUIRE(listSpecies->currentItem()->parent()->text(0) == "Cell");
    // click remove species, then "yes" to confirm
    mwt.addUserAction({"Enter"});
    mwt.start();
    sendMouseClick(btnRemoveSpecies);
    REQUIRE(listSpecies->currentItem()->text(0) == "B_cell");
    REQUIRE(listSpecies->currentItem()->parent()->text(0) == "Cell");
    // click remove species, then "yes" to confirm
    mwt.addUserAction({"Enter"});
    mwt.start();
    sendMouseClick(btnRemoveSpecies);
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
    mwt.addUserAction({"Esc"});
    mwt.start();
    sendMouseClick(btnRemoveSpecies);
    REQUIRE(listSpecies->currentItem()->text(0) == "new species in nucleus");
    REQUIRE(listSpecies->currentItem()->parent()->text(0) == "Nucleus");
    REQUIRE(listSpecies->currentItem()->parent()->childCount() == 3);
    // click remove species, then "yes" to confirm
    mwt.addUserAction({"Enter"});
    mwt.start();
    sendMouseClick(btnRemoveSpecies);
    REQUIRE(listSpecies->currentItem()->text(0) == "A_nucl");
    REQUIRE(listSpecies->currentItem()->parent()->text(0) == "Nucleus");
    REQUIRE(listSpecies->currentItem()->parent()->childCount() == 2);
    // click remove species, then "yes" to confirm
    mwt.addUserAction({"Enter"});
    mwt.start();
    sendMouseClick(btnRemoveSpecies);
    REQUIRE(listSpecies->currentItem()->text(0) == "B_nucl");
    REQUIRE(listSpecies->currentItem()->parent()->text(0) == "Nucleus");
    REQUIRE(listSpecies->currentItem()->parent()->childCount() == 1);
    // click remove species, then "yes" to confirm
    mwt.addUserAction({"Enter"});
    mwt.start();
    sendMouseClick(btnRemoveSpecies);
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

#include "catch_wrapper.hpp"
#include "model_test_utils.hpp"
#include "qt_test_utils.hpp"
#include "regioncolorslabel.hpp"
#include "sme/feature_options.hpp"
#include "sme/model.hpp"
#include "tabfeatures.hpp"
#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>

using namespace sme::test;

TEST_CASE("TabFeatures", "[gui/tabs/features][gui/tabs][gui][features]") {
  sme::model::Model model;
  TabFeatures tab(model);
  tab.show();
  waitFor(&tab);
  auto *listFeatures{tab.findChild<QListWidget *>("listFeatures")};
  REQUIRE(listFeatures != nullptr);
  auto *btnAddFeature{tab.findChild<QPushButton *>("btnAddFeature")};
  REQUIRE(btnAddFeature != nullptr);
  auto *btnRemoveFeature{tab.findChild<QPushButton *>("btnRemoveFeature")};
  REQUIRE(btnRemoveFeature != nullptr);
  auto *txtFeatureName{tab.findChild<QLineEdit *>("txtFeatureName")};
  REQUIRE(txtFeatureName != nullptr);
  auto *cmbCompartment{tab.findChild<QComboBox *>("cmbCompartment")};
  REQUIRE(cmbCompartment != nullptr);
  auto *cmbSpecies{tab.findChild<QComboBox *>("cmbSpecies")};
  REQUIRE(cmbSpecies != nullptr);
  auto *cmbRoiType{tab.findChild<QComboBox *>("cmbRoiType")};
  REQUIRE(cmbRoiType != nullptr);
  auto *stackRoiSettings{tab.findChild<QStackedWidget *>("stackRoiSettings")};
  REQUIRE(stackRoiSettings != nullptr);
  auto *pageRoiAnalytic{tab.findChild<QWidget *>("pageRoiAnalytic")};
  REQUIRE(pageRoiAnalytic != nullptr);
  auto *pageRoiImage{tab.findChild<QWidget *>("pageRoiImage")};
  REQUIRE(pageRoiImage != nullptr);
  auto *pageRoiDepth{tab.findChild<QWidget *>("pageRoiDepth")};
  REQUIRE(pageRoiDepth != nullptr);
  auto *pageRoiAxis{tab.findChild<QWidget *>("pageRoiAxis")};
  REQUIRE(pageRoiAxis != nullptr);
  auto *lblBuiltInThicknessLabel{
      tab.findChild<QLabel *>("lblBuiltInThicknessLabel")};
  REQUIRE(lblBuiltInThicknessLabel != nullptr);
  auto *spnBuiltInThickness{tab.findChild<QSpinBox *>("spnBuiltInThickness")};
  REQUIRE(spnBuiltInThickness != nullptr);
  auto *lblBuiltInAxisLabel{tab.findChild<QLabel *>("lblBuiltInAxisLabel")};
  REQUIRE(lblBuiltInAxisLabel != nullptr);
  auto *cmbBuiltInAxis{tab.findChild<QComboBox *>("cmbBuiltInAxis")};
  REQUIRE(cmbBuiltInAxis != nullptr);
  auto *cmbReduction{tab.findChild<QComboBox *>("cmbReduction")};
  REQUIRE(cmbReduction != nullptr);
  auto *txtExpression{tab.findChild<QLineEdit *>("txtExpression")};
  REQUIRE(txtExpression != nullptr);
  auto *btnEditExpression{tab.findChild<QPushButton *>("btnEditExpression")};
  REQUIRE(btnEditExpression != nullptr);
  auto *btnImportImage{tab.findChild<QPushButton *>("btnImportImage")};
  REQUIRE(btnImportImage != nullptr);
  auto *spnNRegions{tab.findChild<QSpinBox *>("spnNRegions")};
  REQUIRE(spnNRegions != nullptr);
  auto *lblRegionColors{tab.findChild<RegionColorsLabel *>("lblRegionColors")};
  REQUIRE(lblRegionColors != nullptr);
  auto *gridFeatureSettings{
      tab.findChild<QGridLayout *>("gridFeatureSettings")};
  REQUIRE(gridFeatureSettings != nullptr);
  ModalWidgetTimer mwt;
  SECTION("very simple model") {
    model = getExampleModel(Mod::VerySimpleModel);
    while (model.getFeatures().size() > 0) {
      model.getFeatures().remove(0);
    }
    tab.loadModelData();
    REQUIRE(listFeatures->count() == 0);
    REQUIRE(btnAddFeature->isEnabled() == true);
    REQUIRE(btnRemoveFeature->isEnabled() == false);
    REQUIRE(txtFeatureName->isEnabled() == false);
    // all ROI widgets disabled when no feature selected
    REQUIRE(txtExpression->isEnabled() == false);
    REQUIRE(btnEditExpression->isEnabled() == false);
    REQUIRE(spnNRegions->isEnabled() == false);
    REQUIRE(lblRegionColors->isEnabled() == false);
    REQUIRE(lblRegionColors->getNumberOfRegions() == 0);
    REQUIRE(cmbRoiType->isEnabled() == false);
    REQUIRE(stackRoiSettings->isEnabled() == false);
    REQUIRE(btnImportImage->isEnabled() == false);
    REQUIRE(lblBuiltInThicknessLabel->isEnabled() == false);
    REQUIRE(spnBuiltInThickness->isEnabled() == false);
    REQUIRE(lblBuiltInAxisLabel->isEnabled() == false);
    REQUIRE(cmbBuiltInAxis->isEnabled() == false);
    // add feature - defaults to analytic ROI
    mwt.addUserAction({"f", "1"});
    mwt.start();
    sendMouseClick(btnAddFeature);
    REQUIRE(listFeatures->count() == 1);
    REQUIRE(listFeatures->currentItem()->text() == "f1");
    REQUIRE(txtFeatureName->isEnabled() == true);
    REQUIRE(txtFeatureName->text() == "f1");
    REQUIRE(cmbCompartment->isEnabled() == true);
    REQUIRE(cmbCompartment->count() > 0);
    REQUIRE(cmbSpecies->isEnabled() == true);
    REQUIRE(cmbReduction->isEnabled() == true);
    REQUIRE(btnRemoveFeature->isEnabled() == true);
    REQUIRE(model.getFeatures().size() == 1);
    // default is analytic: expression and labels enabled, others disabled
    REQUIRE(cmbRoiType->isEnabled() == true);
    REQUIRE(cmbRoiType->count() == 4);
    REQUIRE(cmbRoiType->currentText() == "Analytic");
    REQUIRE(stackRoiSettings->currentWidget() == pageRoiAnalytic);
    REQUIRE(txtExpression->isEnabled() == true);
    REQUIRE(txtExpression->isReadOnly() == true);
    REQUIRE(txtExpression->text() == "1");
    REQUIRE(btnEditExpression->isEnabled() == true);
    REQUIRE(spnNRegions->isEnabled() == true);
    REQUIRE(spnNRegions->value() == 1);
    int nRegionsRow{};
    int nRegionsColumn{};
    int nRegionsRowSpan{};
    int nRegionsColumnSpan{};
    gridFeatureSettings->getItemPosition(
        gridFeatureSettings->indexOf(spnNRegions), &nRegionsRow,
        &nRegionsColumn, &nRegionsRowSpan, &nRegionsColumnSpan);
    int regionColorsRow{};
    int regionColorsColumn{};
    int regionColorsRowSpan{};
    int regionColorsColumnSpan{};
    gridFeatureSettings->getItemPosition(
        gridFeatureSettings->indexOf(lblRegionColors), &regionColorsRow,
        &regionColorsColumn, &regionColorsRowSpan, &regionColorsColumnSpan);
    REQUIRE(nRegionsRow == regionColorsRow);
    REQUIRE(nRegionsColumn < regionColorsColumn);
    REQUIRE(nRegionsColumnSpan == 1);
    REQUIRE(lblRegionColors->isEnabled() == true);
    REQUIRE(lblRegionColors->getNumberOfRegions() == 1);
    REQUIRE(btnImportImage->isEnabled() == false);
    REQUIRE(spnBuiltInThickness->isEnabled() == false);
    REQUIRE(cmbBuiltInAxis->isEnabled() == false);
    spnNRegions->setValue(3);
    REQUIRE(model.getFeatures().getFeatures()[0].roi.numRegions == 3);
    REQUIRE(lblRegionColors->getNumberOfRegions() == 3);
    // edit ROI analytic expression via dialog
    mwt.addUserAction({"Delete", "2"});
    mwt.start();
    sendMouseClick(btnEditExpression);
    REQUIRE(txtExpression->text() == "2");
    REQUIRE(model.getFeatures().getFeatures()[0].roi.expression == "2");
    // edit name
    txtFeatureName->setFocus();
    sendKeyEvents(txtFeatureName,
                  {"Backspace", "Backspace", "m", "y", "f", "Enter"});
    REQUIRE(txtFeatureName->text() == "myf");
    REQUIRE(listFeatures->currentItem()->text() == "myf");
    // change reduction
    cmbReduction->setCurrentIndex(1); // Sum
    REQUIRE(model.getFeatures().getFeatures()[0].reduction ==
            sme::simulate::ReductionOp::Sum);
    // check that the associations are correct and that we can switch to the
    // quantile stuff too
    REQUIRE(cmbReduction->count() == 7);
    REQUIRE(cmbReduction->itemText(4) == "First quantile");
    REQUIRE(cmbReduction->itemText(5) == "Median");
    REQUIRE(cmbReduction->itemText(6) == "Third quantile");

    cmbReduction->setCurrentIndex(4);
    REQUIRE(model.getFeatures().getFeatures()[0].reduction ==
            sme::simulate::ReductionOp::FirstQuantile);

    cmbReduction->setCurrentIndex(5);
    REQUIRE(model.getFeatures().getFeatures()[0].reduction ==
            sme::simulate::ReductionOp::Median);

    cmbReduction->setCurrentIndex(6);
    REQUIRE(model.getFeatures().getFeatures()[0].reduction ==
            sme::simulate::ReductionOp::ThirdQuantile);

    // switch to image ROI: import button enabled, labels remain editable
    cmbRoiType->setCurrentIndex(cmbRoiType->findText("Image"));
    REQUIRE(model.getFeatures().getFeatures()[0].roi.roiType ==
            sme::simulate::RoiType::Image);
    REQUIRE(stackRoiSettings->currentWidget() == pageRoiImage);
    REQUIRE(txtExpression->isEnabled() == false);
    REQUIRE(btnEditExpression->isEnabled() == false);
    REQUIRE(spnNRegions->isEnabled() == true);
    REQUIRE(btnImportImage->isEnabled() == true);
    REQUIRE(spnBuiltInThickness->isEnabled() == false);
    REQUIRE(cmbBuiltInAxis->isEnabled() == false);
    // switch to depth ROI: thickness enabled, labels remain editable
    cmbRoiType->setCurrentIndex(cmbRoiType->findText("Depth"));
    REQUIRE(model.getFeatures().getFeatures()[0].roi.roiType ==
            sme::simulate::RoiType::Depth);
    REQUIRE(stackRoiSettings->currentWidget() == pageRoiDepth);
    REQUIRE(txtExpression->isEnabled() == false);
    REQUIRE(btnEditExpression->isEnabled() == false);
    REQUIRE(spnNRegions->isEnabled() == true);
    REQUIRE(btnImportImage->isEnabled() == false);
    REQUIRE(lblBuiltInThicknessLabel->isEnabled() == true);
    REQUIRE(spnBuiltInThickness->isEnabled() == true);
    REQUIRE(spnBuiltInThickness->value() == 1);
    spnBuiltInThickness->setValue(2);
    REQUIRE(sme::simulate::getRoiParameterInt(
                model.getFeatures().getFeatures()[0].roi,
                sme::simulate::roi_param::depthThicknessVoxels, 1) == 2);
    REQUIRE(cmbBuiltInAxis->isEnabled() == false);
    // switch to axis slices ROI: axis enabled, thickness disabled
    cmbRoiType->setCurrentIndex(cmbRoiType->findText("Axis slices"));
    REQUIRE(model.getFeatures().getFeatures()[0].roi.roiType ==
            sme::simulate::RoiType::AxisSlices);
    REQUIRE(stackRoiSettings->currentWidget() == pageRoiAxis);
    REQUIRE(txtExpression->isEnabled() == false);
    REQUIRE(btnEditExpression->isEnabled() == false);
    REQUIRE(spnNRegions->isEnabled() == true);
    REQUIRE(btnImportImage->isEnabled() == false);
    REQUIRE(spnBuiltInThickness->isEnabled() == false);
    REQUIRE(lblBuiltInAxisLabel->isEnabled() == true);
    REQUIRE(cmbBuiltInAxis->isEnabled() == true);
    REQUIRE(cmbBuiltInAxis->count() == 3);
    REQUIRE(cmbBuiltInAxis->currentText() == "x");
    cmbBuiltInAxis->setCurrentIndex(cmbBuiltInAxis->findText("y"));
    REQUIRE(sme::simulate::getRoiParameterInt(
                model.getFeatures().getFeatures()[0].roi,
                sme::simulate::roi_param::axis, 0) == 1);
    // switch to analytic ROI
    cmbRoiType->setCurrentIndex(cmbRoiType->findText("Analytic"));
    REQUIRE(model.getFeatures().getFeatures()[0].roi.roiType ==
            sme::simulate::RoiType::Analytic);
    REQUIRE(stackRoiSettings->currentWidget() == pageRoiAnalytic);
    REQUIRE(txtExpression->isEnabled() == true);
    REQUIRE(btnEditExpression->isEnabled() == true);
    REQUIRE(spnNRegions->isEnabled() == true);
    REQUIRE(btnImportImage->isEnabled() == false);
    REQUIRE(spnBuiltInThickness->isEnabled() == false);
    REQUIRE(cmbBuiltInAxis->isEnabled() == false);
    // add second feature
    mwt.addUserAction({"f", "2"});
    mwt.start();
    sendMouseClick(btnAddFeature);
    REQUIRE(listFeatures->count() == 2);
    REQUIRE(model.getFeatures().size() == 2);
    txtFeatureName->setFocus();
    txtFeatureName->clear();
    sendKeyEvents(txtFeatureName, {"m", "y", "f", "Enter"});
    REQUIRE(txtFeatureName->text() == "myf_");
    REQUIRE(listFeatures->currentItem()->text() == "myf_");
    REQUIRE(model.getFeatures().getFeatures()[1].name == "myf_");
    // remove feature & confirm
    mwt.addUserAction({"Enter"});
    mwt.start();
    sendMouseClick(btnRemoveFeature);
    REQUIRE(listFeatures->count() == 1);
    REQUIRE(model.getFeatures().size() == 1);
    // remove feature & cancel
    mwt.addUserAction({"Esc"});
    mwt.start();
    sendMouseClick(btnRemoveFeature);
    REQUIRE(listFeatures->count() == 1);
    REQUIRE(model.getFeatures().size() == 1);
    // remove feature & confirm
    mwt.addUserAction({"Enter"});
    mwt.start();
    sendMouseClick(btnRemoveFeature);
    REQUIRE(listFeatures->count() == 0);
    REQUIRE(model.getFeatures().size() == 0);
    REQUIRE(btnRemoveFeature->isEnabled() == false);
    REQUIRE(txtFeatureName->isEnabled() == false);
  }
}

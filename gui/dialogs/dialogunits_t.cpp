#include "catch_wrapper.hpp"
#include "dialogunits.hpp"
#include "model_test_utils.hpp"
#include "qt_test_utils.hpp"
#include "sme/model.hpp"
#include <QComboBox>
#include <QPushButton>

using namespace sme::test;

struct DialogUnitsWidgets {
  explicit DialogUnitsWidgets(const DialogUnits *dialog) {
    GET_DIALOG_WIDGET(QComboBox, cmbTime);
    GET_DIALOG_WIDGET(QComboBox, cmbLength);
    GET_DIALOG_WIDGET(QComboBox, cmbVolume);
    GET_DIALOG_WIDGET(QComboBox, cmbAmount);
    GET_DIALOG_WIDGET(QPushButton, btnEditTime);
    GET_DIALOG_WIDGET(QPushButton, btnEditLength);
    GET_DIALOG_WIDGET(QPushButton, btnEditVolume);
    GET_DIALOG_WIDGET(QPushButton, btnEditAmount);
  }
  QComboBox *cmbTime;
  QComboBox *cmbLength;
  QComboBox *cmbVolume;
  QComboBox *cmbAmount;
  QPushButton *btnEditTime;
  QPushButton *btnEditLength;
  QPushButton *btnEditVolume;
  QPushButton *btnEditAmount;
};

TEST_CASE("DialogUnits", "[gui/dialogs/units][gui/dialogs][gui][units]") {
  auto m{getExampleModel(Mod::ABtoC)};
  auto units = m.getUnits();
  units.setTimeIndex(0);
  units.setLengthIndex(3);
  units.setVolumeIndex(2);
  units.setAmountIndex(1);
  DialogUnits dia(units);
  dia.show();
  DialogUnitsWidgets widgets(&dia);
  // initial units
  REQUIRE(dia.getTimeUnitIndex() == 0);
  REQUIRE(dia.getLengthUnitIndex() == 3);
  REQUIRE(dia.getVolumeUnitIndex() == 2);
  REQUIRE(dia.getAmountUnitIndex() == 1);
  // select 2nd unit for each
  widgets.cmbTime->setCurrentIndex(1);
  widgets.cmbLength->setCurrentIndex(1);
  widgets.cmbVolume->setCurrentIndex(1);
  widgets.cmbAmount->setCurrentIndex(1);
  REQUIRE(dia.getTimeUnitIndex() == 1);
  REQUIRE(dia.getLengthUnitIndex() == 1);
  REQUIRE(dia.getVolumeUnitIndex() == 1);
  REQUIRE(dia.getAmountUnitIndex() == 1);
  // select last unit for each
  widgets.cmbTime->setCurrentIndex(widgets.cmbTime->count() - 1);
  widgets.cmbLength->setCurrentIndex(widgets.cmbLength->count() - 1);
  widgets.cmbVolume->setCurrentIndex(widgets.cmbVolume->count() - 1);
  widgets.cmbAmount->setCurrentIndex(widgets.cmbAmount->count() - 1);
  REQUIRE(dia.getTimeUnitIndex() == units.getTimeUnits().size() - 1);
  REQUIRE(dia.getLengthUnitIndex() == units.getLengthUnits().size() - 1);
  REQUIRE(dia.getVolumeUnitIndex() == units.getVolumeUnits().size() - 1);
  REQUIRE(dia.getAmountUnitIndex() == units.getAmountUnits().size() - 1);
}

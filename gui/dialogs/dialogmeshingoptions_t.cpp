#include "catch_wrapper.hpp"
#include "dialogmeshingoptions.hpp"
#include "qt_test_utils.hpp"
#include <QComboBox>

using namespace sme::test;

struct DialogMeshingOptionsWidgets {
  explicit DialogMeshingOptionsWidgets(const DialogMeshingOptions *dialog) {
    GET_DIALOG_WIDGET(QComboBox, cmbBoundarySimplificationType);
  }
  QComboBox *cmbBoundarySimplificationType;
};

TEST_CASE("DialogMeshingOptions", "[gui/dialogs/meshingoptions][gui/"
                                  "dialogs][gui][meshingoptions]") {
  DialogMeshingOptions dia(1);
  dia.show();
  DialogMeshingOptionsWidgets widgets(&dia);
  SECTION("user does nothing: unchanged") {
    REQUIRE(dia.getBoundarySimplificationType() == 1);
  }
  SECTION("user changes value") {
    widgets.cmbBoundarySimplificationType->setCurrentIndex(0);
    REQUIRE(dia.getBoundarySimplificationType() == 0);
  }
}

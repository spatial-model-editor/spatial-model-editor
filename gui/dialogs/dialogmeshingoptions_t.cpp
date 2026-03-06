#include "catch_wrapper.hpp"
#include "dialogmeshingoptions.hpp"
#include "qt_test_utils.hpp"
#include <QComboBox>

using namespace sme::test;

struct DialogMeshingOptionsWidgets {
  explicit DialogMeshingOptionsWidgets(const DialogMeshingOptions *dialog) {
    GET_DIALOG_WIDGET(QComboBox, cmbBoundarySimplificationType);
    GET_DIALOG_WIDGET(QComboBox, cmbMeshSourceType);
  }
  QComboBox *cmbBoundarySimplificationType;
  QComboBox *cmbMeshSourceType;
};

TEST_CASE("DialogMeshingOptions", "[gui/dialogs/meshingoptions][gui/"
                                  "dialogs][gui][meshingoptions]") {
  DialogMeshingOptions dia(1, 0);
  dia.show();
  DialogMeshingOptionsWidgets widgets(&dia);
  SECTION("user does nothing: unchanged") {
    REQUIRE(dia.getBoundarySimplificationType() == 1);
    REQUIRE(dia.getMeshSourceType() == 0);
  }
  SECTION("user changes value") {
    widgets.cmbBoundarySimplificationType->setCurrentIndex(0);
    widgets.cmbMeshSourceType->setCurrentIndex(1);
    REQUIRE(dia.getBoundarySimplificationType() == 0);
    REQUIRE(dia.getMeshSourceType() == 1);
  }
}

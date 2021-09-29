#include "catch_wrapper.hpp"
#include "dialogmeshingoptions.hpp"
#include "qt_test_utils.hpp"

using namespace sme::test;

TEST_CASE("DialogMeshingOptions", "[gui/dialogs/meshingoptions][gui/"
                                  "dialogs][gui][meshingoptions]") {
  DialogMeshingOptions dia(1);
  ModalWidgetTimer mwt;
  SECTION("user does nothing: unchanged") {
    mwt.addUserAction();
    mwt.start();
    dia.exec();
    REQUIRE(dia.getBoundarySimplificationType() == 1);
  }
  SECTION("user changes value") {
    mwt.addUserAction({"Up"});
    mwt.start();
    dia.exec();
    REQUIRE(dia.getBoundarySimplificationType() == 0);
  }
}

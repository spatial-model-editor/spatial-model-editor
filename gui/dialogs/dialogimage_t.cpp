#include "catch_wrapper.hpp"
#include "dialogimage.hpp"
#include "qt_test_utils.hpp"
#include <QLabel>

struct DialogImageWidgets {
  explicit DialogImageWidgets(const DialogImage *dialog) {
    GET_DIALOG_WIDGET(QLabel, lblMessage);
    GET_DIALOG_WIDGET(QLabel, lblImage);
  }
  QLabel *lblMessage;
  QLabel *lblImage;
};

TEST_CASE("DialogImage", "[gui/dialogs/image][gui/dialogs][gui][image]") {
  SECTION("Valid ImageStack") {
    sme::common::ImageStack imageStack{{QImage(":/icon/icon128.png")}};
    QString title("my title");
    QString message("see image below:");
    DialogImage dia(nullptr, title, message, imageStack);
    dia.show();
    DialogImageWidgets widgets(&dia);
    REQUIRE(dia.windowTitle() == title);
    REQUIRE(widgets.lblMessage->text() == message);
  }
  SECTION("Empty ImageStack") {
    sme::common::ImageStack imageStack{};
    QString title("my title");
    QString message("see image below:");
    DialogImage dia(nullptr, title, message, imageStack);
    dia.show();
    DialogImageWidgets widgets(&dia);
    REQUIRE(dia.windowTitle() == title);
    REQUIRE(widgets.lblMessage->text() == message);
  }
}

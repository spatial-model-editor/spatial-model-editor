#include "catch_wrapper.hpp"
#include "dialogimage.hpp"
#include <QLabel>

TEST_CASE("DialogImage", "[gui/dialogs/image][gui/dialogs][gui][image]") {
  QImage image(":/icon/icon128.png");
  QString title("my title");
  QString message("see image below:");
  DialogImage dia(nullptr, title, message, image);
  auto *lblMessage{dia.findChild<QLabel *>("lblMessage")};
  REQUIRE(lblMessage != nullptr);
  auto *lblImage{dia.findChild<QLabel *>("lblImage")};
  REQUIRE(lblImage != nullptr);
  REQUIRE(dia.windowTitle() == title);
  REQUIRE(lblMessage->text() == message);
}

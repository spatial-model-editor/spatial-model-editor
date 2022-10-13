#include "catch_wrapper.hpp"
#include "guiutils.hpp"
#include "qt_test_utils.hpp"
#include <QImage>

using namespace sme::test;

TEST_CASE("GuiUtils: getImageFromUser", "[gui/guiutils][gui]") {
  ModalWidgetTimer mwt;
  QRgb c0{qRgb(12, 43, 77)};
  QRgb c1{qRgb(92, 11, 177)};
  QImage img(123, 88, QImage::Format_RGB32);
  img.fill(c0);
  img.setPixel(23, 32, c1);
  img.save("tmp.png");
  mwt.addUserAction({"t", "m", "p", ".", "p", "n", "g"}, true);
  mwt.start();
  auto importedImg{getImageFromUser(nullptr)};
  REQUIRE(importedImg.volume().width() == img.width());
  REQUIRE(importedImg.volume().height() == img.height());
  REQUIRE(importedImg.volume().depth() == 1);
  REQUIRE(importedImg[0].pixel(23, 32) == img.pixel(23, 32));
  REQUIRE(importedImg[0].pixel(99, 12) == img.pixel(99, 12));
}

#include <QSignalSpy>

#include "catch.hpp"
#include "catch_qt_ostream.hpp"

#include "qlabelmousetracker.h"

TEST_CASE("qlabelmousetracker: todo", "[qlabelmousetracker]") {
  QLabelMouseTracker mouseTracker;
  QImage img(1, 1, QImage::Format_RGB32);
  QRgb col = QColor(12, 243, 154).rgba();
  img.setPixel(0, 0, col);
  mouseTracker.setImage(img);
  mouseTracker.show();
  REQUIRE(mouseTracker.getImage().size() == img.size());
  REQUIRE(mouseTracker.getImage().pixel(0, 0) == img.pixel(0, 0));
}

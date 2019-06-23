#include <QtTest>

#include "catch.hpp"
#include "catch_qt_ostream.hpp"

#include "qlabelmousetracker.h"

TEST_CASE("qlabelmousetracker", "[qlabelmousetracker]") {
  QLabelMouseTracker mouseTracker;
  QImage img(1, 1, QImage::Format_RGB32);
  QRgb col = QColor(12, 243, 154).rgba();
  img.setPixel(0, 0, col);
  mouseTracker.setImage(img);
  mouseTracker.resize(10, 10);
  mouseTracker.show();
  REQUIRE(mouseTracker.getImage().size() == img.size());
  REQUIRE(mouseTracker.getImage().pixel(0, 0) == img.pixel(0, 0));
  REQUIRE(mouseTracker.getColour() == col);

  std::vector<QRgb> clicks;
  QObject::connect(&mouseTracker, &QLabelMouseTracker::mouseClicked,
                   [&clicks](QRgb c) { clicks.push_back(c); });

  REQUIRE(clicks.size() == 0);
  // click on image:
  QTest::mouseClick(&mouseTracker, Qt::LeftButton, Qt::KeyboardModifiers(),
                    QPoint(mouseTracker.pos()) + QPoint(1, 1));
  REQUIRE(clicks.size() == 1);
  REQUIRE(clicks.back() == col);
  REQUIRE(mouseTracker.getColour() == col);
  // click again on image:
  QTest::mouseClick(&mouseTracker, Qt::LeftButton, Qt::KeyboardModifiers(),
                    QPoint(mouseTracker.pos() + QPoint(4, 3)));
  REQUIRE(clicks.size() == 2);
  REQUIRE(clicks.back() == col);
  REQUIRE(mouseTracker.getColour() == col);
}

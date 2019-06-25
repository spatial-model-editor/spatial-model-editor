#include <QtTest>

#include "catch.hpp"
#include "catch_qt_ostream.hpp"

#include "qlabelmousetracker.h"

constexpr int mouseDelay = 50;

TEST_CASE("qlabelmousetracker", "[qlabelmousetracker][gui]") {
  QLabelMouseTracker mouseTracker;
  QImage img(1, 1, QImage::Format_RGB32);
  QRgb col = QColor(12, 243, 154).rgba();
  img.setPixel(0, 0, col);
  mouseTracker.setImage(img);
  mouseTracker.resize(100, 100);
  mouseTracker.show();
  QTest::qWait(100);

  REQUIRE(mouseTracker.getImage().size() == img.size());
  REQUIRE(mouseTracker.getImage().pixel(0, 0) == img.pixel(0, 0));
  REQUIRE(mouseTracker.getColour() == col);

  std::vector<QRgb> clicks;
  QObject::connect(&mouseTracker, &QLabelMouseTracker::mouseClicked,
                   [&clicks](QRgb c) { clicks.push_back(c); });

  REQUIRE(clicks.size() == 0);

  //  QApplication::setActiveWindow(&mouseTracker);
  // move mouse over image
  QTest::mouseMove(mouseTracker.windowHandle(), QPoint(1, 1), mouseDelay);
  QTest::mouseMove(mouseTracker.windowHandle(), QPoint(1, 11), mouseDelay);
  // click on image
  QTest::mouseClick(&mouseTracker, Qt::LeftButton, Qt::KeyboardModifiers(),
                    QPoint(2, 3), mouseDelay);
  REQUIRE(clicks.size() == 1);
  REQUIRE(clicks.back() == col);
  REQUIRE(mouseTracker.getColour() == col);
  // click again elsewhere on image:
  // move mouse over image
  QTest::mouseMove(mouseTracker.windowHandle(), QPoint(33, 44), mouseDelay);
  QTest::mouseMove(mouseTracker.windowHandle(), QPoint(55, 54), mouseDelay);
  // click on image
  QTest::mouseClick(&mouseTracker, Qt::LeftButton, Qt::KeyboardModifiers(),
                    QPoint(66, 81), mouseDelay);
  REQUIRE(clicks.size() == 2);
  REQUIRE(clicks.back() == col);
  REQUIRE(mouseTracker.getColour() == col);
}

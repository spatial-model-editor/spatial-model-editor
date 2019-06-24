#include <QtTest>

#include "catch.hpp"
#include "catch_qt_ostream.hpp"

#include "qlabelmousetracker.h"

static void sendMouseMove(QWidget *widget, const QPoint &point,
                          Qt::MouseButton button = Qt::NoButton,
                          Qt::MouseButtons buttons = 0) {
  QTest::mouseMove(widget, point);
  QTest::qWait(40);
  QMouseEvent event(QEvent::MouseMove, point, button, buttons, 0);
  QApplication::sendEvent(widget, &event);
  QTest::qWait(40);
  QApplication::processEvents();
  QTest::qWait(40);
}

TEST_CASE("qlabelmousetracker", "[qlabelmousetracker][gui]") {
  QLabelMouseTracker mouseTracker;
  QImage img(1, 1, QImage::Format_RGB32);
  QRgb col = QColor(12, 243, 154).rgba();
  img.setPixel(0, 0, col);
  mouseTracker.setImage(img);
  mouseTracker.resize(100, 100);
  mouseTracker.show();
  QTest::qWait(100);

  QCursor::setPos(500, 500);

  REQUIRE(mouseTracker.getImage().size() == img.size());
  REQUIRE(mouseTracker.getImage().pixel(0, 0) == img.pixel(0, 0));
  REQUIRE(mouseTracker.getColour() == col);

  std::vector<QRgb> clicks;
  QObject::connect(&mouseTracker, &QLabelMouseTracker::mouseClicked,
                   [&clicks](QRgb c) { clicks.push_back(c); });

  REQUIRE(clicks.size() == 0);
  // click on image:
  QApplication::setActiveWindow(&mouseTracker);
  // QTest::mouseMove(mouseTracker.windowHandle(), QPoint(1, 1));
  sendMouseMove(&mouseTracker, QPoint(3, 6));
  QTest::qWait(100);
  QTest::mouseMove(mouseTracker.windowHandle(), QPoint(1, 11));
  QApplication::processEvents();
  QTest::qWait(100);
  QTest::mouseClick(&mouseTracker, Qt::LeftButton);
  REQUIRE(clicks.size() == 1);
  REQUIRE(clicks.back() == col);
  REQUIRE(mouseTracker.getColour() == col);
  // click again elsewhere on image:
  QTest::mouseMove(&mouseTracker, QPoint(66, 32));
  QTest::qWait(100);
  QApplication::processEvents();
  QTest::mouseClick(&mouseTracker, Qt::LeftButton);
  REQUIRE(clicks.size() == 2);
  REQUIRE(clicks.back() == col);
  REQUIRE(mouseTracker.getColour() == col);
}

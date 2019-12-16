#include <QApplication>
#include <QDebug>

#include "catch_wrapper.hpp"
#include "logger.hpp"
#include "qlabelmousetracker.hpp"
#include "qt_test_utils.hpp"

SCENARIO("QLabelMouseTracker", "[gui][widgets][qlabelmousetracker]") {
  GIVEN("single pixel, single colour image") {
    QLabelMouseTracker mouseTracker;
    QImage img(1, 1, QImage::Format_RGB32);
    QRgb col = QColor(12, 243, 154).rgba();
    img.setPixel(0, 0, col);
    mouseTracker.show();
    mouseTracker.resize(100, 100);
    // NB: window managers might interfere with above resizing
    // so wait a bit until the size is correct and stable
    wait();
    mouseTracker.setImage(img);
    REQUIRE(mouseTracker.getImage().size() == img.size());
    REQUIRE(mouseTracker.getImage().pixel(0, 0) == img.pixel(0, 0));

    std::vector<QRgb> clicks;
    QObject::connect(&mouseTracker, &QLabelMouseTracker::mouseClicked,
                     [&clicks](QRgb c) { clicks.push_back(c); });
    REQUIRE(clicks.size() == 0);
    WHEN("mouse clicked on image") {
      // move mouse over image
      sendMouseMove(&mouseTracker, {2, 3});
      // click on image
      sendMouseClick(&mouseTracker, {2, 3});
      THEN("register click, get colour of pixel") {
        REQUIRE(clicks.size() == 1);
        REQUIRE(clicks.back() == col);
        REQUIRE(mouseTracker.getColour() == col);
      }
    }
    WHEN("mouse clicked twice on image") {
      // click on image
      sendMouseMove(&mouseTracker, {66, 81});
      sendMouseClick(&mouseTracker, {66, 81});
      sendMouseMove(&mouseTracker, {55, 11});
      sendMouseClick(&mouseTracker, {55, 11});
      THEN("register clicks, get colour of last pixel clicked") {
        REQUIRE(clicks.size() == 2);
        REQUIRE(clicks.back() == col);
        REQUIRE(mouseTracker.getColour() == col);
      }
    }
  }
  GIVEN("3x3 pixel, 4 colour image") {
    // pixel colours:
    // 2 1 1
    // 3 4 1
    // 3 1 1
    QLabelMouseTracker mouseTracker;
    QImage img(3, 3, QImage::Format_RGB32);
    QRgb col1 = QColor(12, 243, 154).rgba();
    QRgb col2 = QColor(34, 92, 14).rgba();
    QRgb col3 = QColor(88, 43, 91).rgba();
    QRgb col4 = QColor(108, 13, 55).rgba();
    img.fill(col1);
    img.setPixel(0, 0, col2);
    img.setPixel(0, 1, col3);
    img.setPixel(0, 2, col3);
    img.setPixel(1, 1, col4);
    mouseTracker.setImage(img);
    mouseTracker.show();
    mouseTracker.resize(100, 100);
    wait();

    CAPTURE(col1);
    CAPTURE(col2);
    CAPTURE(col3);
    CAPTURE(col4);
    REQUIRE(mouseTracker.getImage().size() == img.size());
    REQUIRE(mouseTracker.getImage().pixel(0, 0) == img.pixel(0, 0));
    REQUIRE(mouseTracker.getImage().pixel(2, 2) == img.pixel(2, 2));
    int imgDisplaySize = std::min(mouseTracker.width(), mouseTracker.height());
    CAPTURE(imgDisplaySize);

    std::vector<QRgb> clicks;
    QObject::connect(&mouseTracker, &QLabelMouseTracker::mouseClicked,
                     [&clicks](QRgb c) { clicks.push_back(c); });
    REQUIRE(clicks.size() == 0);
    WHEN("mouse click (0,1)") {
      sendMouseMove(&mouseTracker, {99, 99});
      sendMouseClick(&mouseTracker, {0, 1});
      THEN("pix (0,0) <-> col2") {
        REQUIRE(clicks.size() == 1);
        REQUIRE(clicks.back() == col2);
        REQUIRE(mouseTracker.getColour() == col2);
      }
    }
    WHEN("mouse click (3,7)") {
      sendMouseClick(&mouseTracker, {3, 7});
      THEN("pix (0,0) <-> col2") {
        REQUIRE(clicks.size() == 1);
        REQUIRE(clicks.back() == col2);
        REQUIRE(mouseTracker.getColour() == col2);
      }
    }
    WHEN("mouse click (5,44)") {
      THEN("pix (0,1) <-> col3") {
        sendMouseClick(&mouseTracker,
                       QPoint(static_cast<int>(0.05 * imgDisplaySize),
                              static_cast<int>(0.44 * imgDisplaySize)));
        REQUIRE(clicks.size() == 1);
        REQUIRE(clicks.back() == col3);
        REQUIRE(mouseTracker.getColour() == col3);
      }
    }
    WHEN("mouse click (5,84)") {
      THEN("pix (0,2) <-> col3") {
        sendMouseClick(&mouseTracker,
                       QPoint(static_cast<int>(0.05 * imgDisplaySize),
                              static_cast<int>(0.84 * imgDisplaySize)));
        REQUIRE(clicks.size() == 1);
        REQUIRE(clicks.back() == col3);
        REQUIRE(mouseTracker.getColour() == col3);
      }
    }
    WHEN("mouse clicks (99,99), (5,84), (50,50)") {
      sendMouseClick(&mouseTracker, {99, 99});
      sendMouseMove(&mouseTracker, {79, 19});
      sendMouseClick(&mouseTracker, {5, 84});
      sendMouseClick(&mouseTracker, {50, 50});
      THEN("pix (2,2),(0,2),(1,1) <-> col1,col3,col4") {
        REQUIRE(clicks.size() == 3);
        REQUIRE(clicks[0] == col1);
        REQUIRE(clicks[1] == col3);
        REQUIRE(clicks[2] == col4);
        REQUIRE(mouseTracker.getColour() == col4);
      }
    }
    WHEN("resized to 600x600, mouseclick (300,300)") {
      mouseTracker.resize(600, 600);
      wait();
      sendMouseClick(&mouseTracker, {300, 300});
      THEN("pix (1,1) <-> col4") {
        REQUIRE(clicks.size() == 1);
        REQUIRE(clicks.back() == col4);
        REQUIRE(mouseTracker.getColour() == col4);
      }
    }
    WHEN("resized to 6x6, mouseclick (1,3)") {
      mouseTracker.resize(6, 6);
      wait();
      sendMouseClick(&mouseTracker, {1, 3});
      THEN("pix (0,1) <-> col3") {
        REQUIRE(clicks.size() == 1);
        REQUIRE(clicks.back() == col3);
        REQUIRE(mouseTracker.getColour() == col3);
      }
    }
    WHEN("resized to 900x3, mouseclick (2,2)") {
      mouseTracker.resize(900, 3);
      wait();
      sendMouseClick(&mouseTracker, {2, 2});
      THEN("pix (2,0) <-> col1") {
        REQUIRE(clicks.size() == 1);
        REQUIRE(clicks.back() == col1);
        REQUIRE(mouseTracker.getColour() == col1);
      }
    }
  }
  GIVEN("image and mask") {
    QLabelMouseTracker mouseTracker;
    QImage img(":/geometry/circle-100x100.png");
    QImage mask(":/geometry/concave-cell-nucleus-100x100.png");
    mouseTracker.setImages({img, mask});
    mouseTracker.show();
    mouseTracker.resize(100, 100);
    wait();

    REQUIRE(mouseTracker.getImage().size() == img.size());
    REQUIRE(mouseTracker.getImage().pixel(0, 0) == img.pixel(0, 0));
    REQUIRE(mouseTracker.getImage().pixel(2, 2) == img.pixel(2, 2));
    REQUIRE(mouseTracker.getMaskImage().size() == mask.size());
    REQUIRE(mouseTracker.getMaskImage().pixel(0, 0) == mask.pixel(0, 0));
    REQUIRE(mouseTracker.getMaskImage().pixel(2, 2) == mask.pixel(2, 2));

    std::vector<QRgb> clicks;
    QObject::connect(&mouseTracker, &QLabelMouseTracker::mouseClicked,
                     [&clicks](QRgb c) { clicks.push_back(c); });
    REQUIRE(clicks.size() == 0);
    WHEN("mouse click (0,1)") {
      sendMouseMove(&mouseTracker, {99, 99});
      sendMouseClick(&mouseTracker, {0, 1});
      REQUIRE(clicks.size() == 1);
      REQUIRE(clicks.back() == 0xff000200);
      REQUIRE(mouseTracker.getColour() == 0xff000200);
      REQUIRE(mouseTracker.getMaskIndex() == 512);
    }
    WHEN("mouse click (50,50)") {
      sendMouseMove(&mouseTracker, {99, 99});
      sendMouseClick(&mouseTracker, {50, 50});
      REQUIRE(clicks.size() == 1);
      REQUIRE(clicks.back() == QColor(144, 97, 193).rgb());
      REQUIRE(mouseTracker.getColour() == QColor(144, 97, 193).rgb());
      REQUIRE(mouseTracker.getMaskIndex() == 12944736);
    }
  }
}

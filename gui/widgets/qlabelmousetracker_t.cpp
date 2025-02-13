#include "catch_wrapper.hpp"
#include "qlabelmousetracker.hpp"
#include "qt_test_utils.hpp"

using namespace sme::test;

static const char *tags{
    "[qlabelmousetracker][gui/widgets/qlabelmousetracker][gui/widgets][gui]"};

TEST_CASE("QLabelMouseTracker: single pixel, single color image", tags) {
  QLabelMouseTracker mouseTracker;
  sme::common::ImageStack imgs{{QImage(1, 1, QImage::Format_RGB32)}};
  QRgb col{qRgb(12, 243, 154)};
  imgs[0].setPixel(0, 0, col);
  mouseTracker.show();
  mouseTracker.resize(100, 100);
  // NB: window managers might interfere with above resizing
  // so wait a bit until the size is correct and stable
  wait();
  mouseTracker.setImage(imgs);
  REQUIRE(mouseTracker.getImage().volume() == imgs.volume());
  REQUIRE(mouseTracker.getImage()[0].pixel(0, 0) == imgs[0].pixel(0, 0));

  std::vector<QRgb> clicks;
  QObject::connect(&mouseTracker, &QLabelMouseTracker::mouseClicked,
                   [&clicks](QRgb c) { clicks.push_back(c); });
  REQUIRE(mouseTracker.getRelativePosition().x() == dbl_approx(0));
  REQUIRE(mouseTracker.getRelativePosition().y() == dbl_approx(0));
  REQUIRE(clicks.size() == 0);

  // move mouse over image
  sendMouseMove(&mouseTracker, {2, 3});
  REQUIRE(mouseTracker.getRelativePosition().x() == dbl_approx(0));
  REQUIRE(mouseTracker.getRelativePosition().y() == dbl_approx(0));
  // click on image
  sendMouseClick(&mouseTracker, {2, 3});
  REQUIRE(clicks.size() == 1);
  REQUIRE(clicks.back() == col);
  REQUIRE(mouseTracker.getColor() == col);

  // two more clicks on image
  sendMouseMove(&mouseTracker, {66, 81});
  sendMouseClick(&mouseTracker, {66, 81});
  REQUIRE(mouseTracker.getRelativePosition().x() == dbl_approx(0));
  REQUIRE(mouseTracker.getRelativePosition().y() == dbl_approx(0));
  sendMouseMove(&mouseTracker, {55, 11});
  sendMouseClick(&mouseTracker, {55, 11});
  REQUIRE(clicks.size() == 3);
  REQUIRE(clicks.back() == col);
  REQUIRE(mouseTracker.getColor() == col);
}

TEST_CASE("QLabelMouseTracker: 3x3 pixel, 4 color image", tags) {
  // pixel colors:
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
  auto imageStack = sme::common::ImageStack{{img}};
  mouseTracker.setImage(imageStack);
  mouseTracker.show();
  mouseTracker.resize(100, 100);
  wait();

  CAPTURE(col1);
  CAPTURE(col2);
  CAPTURE(col3);
  CAPTURE(col4);
  REQUIRE(mouseTracker.getImage().volume() ==
          sme::common::Volume{img.size(), 1});
  REQUIRE(mouseTracker.getImage()[0].pixel(0, 0) == img.pixel(0, 0));
  REQUIRE(mouseTracker.getImage()[0].pixel(2, 2) == img.pixel(2, 2));
  int imgDisplaySize = std::min(mouseTracker.width(), mouseTracker.height());
  CAPTURE(imgDisplaySize);
  std::vector<QRgb> clicks;
  QObject::connect(&mouseTracker, &QLabelMouseTracker::mouseClicked,
                   [&clicks](QRgb c) { clicks.push_back(c); });

  SECTION("Resize QLabelMousetracker widget") {
    REQUIRE(clicks.size() == 0);
    // mouse click (0,1)
    // pix (0,0) <-> col2
    sendMouseMove(&mouseTracker, {99, 99});
    sendMouseClick(&mouseTracker, {0, 1});
    REQUIRE(clicks.size() == 1);
    REQUIRE(clicks.back() == col2);
    REQUIRE(mouseTracker.getColor() == col2);

    // mouse click (3,7)
    // pix (0,0) <-> col2
    sendMouseClick(&mouseTracker, {3, 7});
    REQUIRE(clicks.size() == 2);
    REQUIRE(clicks.back() == col2);
    REQUIRE(mouseTracker.getColor() == col2);

    // mouse click (5,44)
    // pix (0,1) <-> col3
    sendMouseClick(&mouseTracker,
                   QPoint(static_cast<int>(0.05 * imgDisplaySize),
                          static_cast<int>(0.44 * imgDisplaySize)));
    REQUIRE(clicks.size() == 3);
    REQUIRE(clicks.back() == col3);
    REQUIRE(mouseTracker.getColor() == col3);

    // mouse click (5,84)
    // pix (0,2) <-> col3
    sendMouseClick(&mouseTracker,
                   QPoint(static_cast<int>(0.05 * imgDisplaySize),
                          static_cast<int>(0.84 * imgDisplaySize)));
    REQUIRE(clicks.size() == 4);
    REQUIRE(clicks.back() == col3);
    REQUIRE(mouseTracker.getColor() == col3);

    // mouse clicks (99,99), (5,84), (50,50)
    // pix (2,2),(0,2),(1,1) <-> col1,col3,col4
    clicks.clear();
    sendMouseClick(&mouseTracker, {99, 99});
    sendMouseMove(&mouseTracker, {79, 19});
    REQUIRE(mouseTracker.getRelativePosition().x() == dbl_approx(2.0 / 3.0));
    REQUIRE(mouseTracker.getRelativePosition().y() == dbl_approx(0.0));
    sendMouseClick(&mouseTracker, {5, 84});
    sendMouseClick(&mouseTracker, {50, 50});
    REQUIRE(clicks.size() == 3);
    REQUIRE(clicks[0] == col1);
    REQUIRE(clicks[1] == col3);
    REQUIRE(clicks[2] == col4);
    REQUIRE(mouseTracker.getColor() == col4);

    // resized to 600x600, mouseclick (300,300)
    // pix (1,1) <-> col4
    mouseTracker.resize(600, 600);
    wait();
    sendMouseClick(&mouseTracker, {300, 300});
    REQUIRE(clicks.size() == 4);
    REQUIRE(clicks.back() == col4);
    REQUIRE(mouseTracker.getColor() == col4);

    // resized to 6x6, mouseclick (1,3)
    // pix (0,1) <-> col3
    mouseTracker.resize(6, 6);
    wait();
    sendMouseClick(&mouseTracker, {1, 3});
    REQUIRE(clicks.size() == 5);
    REQUIRE(clicks.back() == col3);
    REQUIRE(mouseTracker.getColor() == col3);

    // resized to 900x3, mouseclick (2,2)
    // pix (2,0) <-> col1
    mouseTracker.resize(900, 3);
    wait();
    sendMouseClick(&mouseTracker, {2, 2});
    REQUIRE(clicks.size() == 6);
    REQUIRE(clicks.back() == col1);
    REQUIRE(mouseTracker.getColor() == col1);
  }
  SECTION("Resize physical volume") {
    // 100px x 100px mousetracker widget size:
    // 1:1 aspect ratio physical size
    imageStack.setVoxelSize({0.77, 0.77, 0.77});
    mouseTracker.setImage(imageStack);
    // click on middle (1,1) pixel
    sendMouseClick(&mouseTracker, QPoint(50, 50));
    REQUIRE(clicks.size() == 1);
    REQUIRE(clicks.back() == col4);
    REQUIRE(mouseTracker.getColor() == col4);
    // 2:1 aspect ratio physical size
    imageStack.setVoxelSize({2.0, 1.0, 1.0});
    mouseTracker.setImage(imageStack);
    // click outside geometry
    sendMouseClick(&mouseTracker, QPoint(50, 52));
    REQUIRE(clicks.size() == 1);
    // click on middle (1,1) pixel
    sendMouseClick(&mouseTracker, QPoint(50, 25));
    REQUIRE(clicks.size() == 2);
    REQUIRE(clicks.back() == col4);
    REQUIRE(mouseTracker.getColor() == col4);
    // 5:1 aspect ratio physical size
    imageStack.setVoxelSize({50.0, 10.0, 1.0});
    mouseTracker.setImage(imageStack);
    // click outside geometry
    sendMouseClick(&mouseTracker, QPoint(50, 22));
    REQUIRE(clicks.size() == 2);
    // click on middle (1,1) pixel
    sendMouseClick(&mouseTracker, QPoint(50, 10));
    REQUIRE(clicks.size() == 3);
    REQUIRE(clicks.back() == col4);
    REQUIRE(mouseTracker.getColor() == col4);
    // 1:3 aspect ratio physical size
    imageStack.setVoxelSize({3.0, 9.0, 1.0});
    mouseTracker.setImage(imageStack);
    // click outside geometry
    sendMouseClick(&mouseTracker, QPoint(35, 50));
    REQUIRE(clicks.size() == 3);
    // click on middle (1,1) pixel
    sendMouseClick(&mouseTracker, QPoint(17, 50));
    REQUIRE(clicks.size() == 4);
    REQUIRE(clicks.back() == col4);
    REQUIRE(mouseTracker.getColor() == col4);
    // 1:100000 aspect ratio physical size: display width clipped to single
    // pixel
    imageStack.setVoxelSize({1.0, 100000.0, 1.0});
    mouseTracker.setImage(imageStack);
    // click outside geometry
    sendMouseClick(&mouseTracker, QPoint(3, 50));
    REQUIRE(clicks.size() == 4);
    // click on geometry
    sendMouseClick(&mouseTracker, QPoint(0, 50));
    REQUIRE(clicks.size() == 5);
    // 100000:1 aspect ratio physical size: display height clipped to single
    // pixel
    imageStack.setVoxelSize({100000.0, 1.0, 1.0});
    mouseTracker.setImage(imageStack);
    // click outside geometry
    sendMouseClick(&mouseTracker, QPoint(50, 3));
    REQUIRE(clicks.size() == 5);
    // click on geometry
    sendMouseClick(&mouseTracker, QPoint(50, 0));
    REQUIRE(clicks.size() == 6);
  }
}

TEST_CASE("QLabelMouseTracker: image and mask", tags) {
  QLabelMouseTracker mouseTracker;
  sme::common::ImageStack img{{QImage(":/geometry/circle-100x100.png")}};
  sme::common::ImageStack mask{
      {QImage(":/geometry/concave-cell-nucleus-100x100.png")}};
  mouseTracker.setImages({img, mask});
  mouseTracker.show();
  mouseTracker.resize(100, 100);
  wait();

  REQUIRE(mouseTracker.getImage().volume() == img.volume());
  REQUIRE(mouseTracker.getImage()[0].pixel(0, 0) == img[0].pixel(0, 0));
  REQUIRE(mouseTracker.getImage()[0].pixel(2, 2) == img[0].pixel(2, 2));
  REQUIRE(mouseTracker.getMaskImage().volume() == mask.volume());
  REQUIRE(mouseTracker.getMaskImage()[0].pixel(0, 0) == mask[0].pixel(0, 0));
  REQUIRE(mouseTracker.getMaskImage()[0].pixel(2, 2) == mask[0].pixel(2, 2));

  std::vector<QRgb> clicks;
  QObject::connect(&mouseTracker, &QLabelMouseTracker::mouseClicked,
                   [&clicks](QRgb c) { clicks.push_back(c); });
  REQUIRE(clicks.size() == 0);

  // mouse click (0,1)
  sendMouseMove(&mouseTracker, {99, 99});
  sendMouseClick(&mouseTracker, {0, 1});
  REQUIRE(clicks.size() == 1);
  REQUIRE(clicks.back() == 0xff000200);
  REQUIRE(mouseTracker.getColor() == 0xff000200);
  REQUIRE(mouseTracker.getMaskIndex() == 512);

  // mouse click (50,50)
  sendMouseMove(&mouseTracker, {99, 99});
  sendMouseClick(&mouseTracker, {50, 50});
  REQUIRE(clicks.size() == 2);
  REQUIRE(clicks.back() == QColor(144, 97, 193).rgb());
  REQUIRE(mouseTracker.getColor() == QColor(144, 97, 193).rgb());
  REQUIRE(mouseTracker.getMaskIndex() == 12944736);
}

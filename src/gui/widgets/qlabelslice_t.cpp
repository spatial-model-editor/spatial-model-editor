#include "catch_wrapper.hpp"
#include "logger.hpp"
#include "qlabelslice.hpp"
#include "qt_test_utils.hpp"
#include <QApplication>
#include <QDebug>
#include <QObject>
#include <vector>

SCENARIO("QLabelSlice",
         "[gui/widgets/qlabelslice][gui/widgets][gui][qlabelslice]") {
  GIVEN("10x5 image") {
    QLabelSlice lblSlice;

    std::vector<QPoint> mouseDownPoints;
    std::vector<std::pair<QPoint, QPoint>> slices;
    QObject::connect(
        &lblSlice, &QLabelSlice::mouseDown,
        [&mouseDownPoints](QPoint p) { mouseDownPoints.push_back(p); });
    QObject::connect(&lblSlice, &QLabelSlice::sliceDrawn,
                     [&slices](QPoint start, QPoint end) {
                       slices.push_back({start, end});
                     });
    QImage img(10, 5, QImage::Format_RGB32);
    img.fill(qRgb(12, 66, 99));
    lblSlice.setAspectRatioMode(Qt::AspectRatioMode::IgnoreAspectRatio);
    lblSlice.setTransformationMode(Qt::TransformationMode::FastTransformation);
    lblSlice.show();
    lblSlice.resize(100, 100);
    // NB: window managers might interfere with above resizing
    // so wait a bit until the size is correct and stable
    wait();
    lblSlice.setImage(img);

    // no initial slice set
    REQUIRE(lblSlice.getSlicePixels().empty());
    REQUIRE(lblSlice.getImage().size() == img.size());
    // all pixels partially transparent
    for (int x = 0; x < img.width(); ++x) {
      for (int y = 0; y < img.height(); ++y) {
        REQUIRE(qAlpha(lblSlice.getImage().pixel(x, y)) < 255);
      }
    }

    // set horizontal slice
    lblSlice.setHorizontalSlice(2);
    REQUIRE(lblSlice.getSlicePixels().size() ==
            static_cast<std::size_t>(img.width()));
    for (int x = 0; x < img.width(); ++x) {
      auto p = lblSlice.getSlicePixels()[static_cast<std::size_t>(x)];
      REQUIRE(p == QPoint(x, 2));
      // slice pixels not transparent
      REQUIRE(qAlpha(lblSlice.getImage().pixel(p)) == 255);
    }
    for (int x = 0; x < img.width(); ++x) {
      for (int y = 0; y < img.height(); ++y) {
        if (y != 2) {
          REQUIRE(qAlpha(lblSlice.getImage().pixel(x, y)) < 255);
        }
      }
    }

    // click
    REQUIRE(mouseDownPoints.empty());
    REQUIRE(slices.empty());
    sendMouseClick(&lblSlice, {32, 62});
    REQUIRE(mouseDownPoints.size() == 1);
    REQUIRE(mouseDownPoints.back() == QPoint(3, 3));
    REQUIRE(slices.size() == 1);
    REQUIRE(slices.back() == std::pair<QPoint, QPoint>{{3, 3}, {3, 3}});
    sendMouseClick(&lblSlice, {42, 62});
    REQUIRE(mouseDownPoints.size() == 2);
    REQUIRE(mouseDownPoints.back() == QPoint(4, 3));
    REQUIRE(slices.size() == 2);
    REQUIRE(slices.back() == std::pair<QPoint, QPoint>{{4, 3}, {4, 3}});

    // click and drag
    sendMouseDrag(&lblSlice, {42, 62}, {82, 93});
    REQUIRE(slices.size() == 3);
    REQUIRE(slices.back() == std::pair<QPoint, QPoint>{{4, 3}, {8, 4}});
    sendMouseDrag(&lblSlice, {82, 93}, {42, 62});
    REQUIRE(slices.size() == 4);
    REQUIRE(slices.back() == std::pair<QPoint, QPoint>{{8, 4}, {4, 3}});

    // set vertical slice
    lblSlice.setVerticalSlice(3);
    REQUIRE(lblSlice.getSlicePixels().size() ==
            static_cast<std::size_t>(img.height()));
    for (int y = 0; y < img.height(); ++y) {
      auto p = lblSlice.getSlicePixels()[static_cast<std::size_t>(y)];
      REQUIRE(p == QPoint(3, img.height() - 1 - y));
      // slice pixels not transparent
      REQUIRE(qAlpha(lblSlice.getImage().pixel(p)) == 255);
    }
    for (int x = 0; x < img.width(); ++x) {
      if (x != 3) {
        for (int y = 0; y < img.height(); ++y) {
          REQUIRE(qAlpha(lblSlice.getImage().pixel(x, y)) < 255);
        }
      }
    }

    // set diagonal slice
    lblSlice.setSlice({0, 0}, {9, 4});
    std::vector<QPoint> pixels{{0, 0}, {1, 0}, {2, 0}, {3, 1}, {4, 1},
                               {5, 2}, {6, 2}, {7, 3}, {8, 3}, {9, 4}};
    REQUIRE(lblSlice.getSlicePixels().size() == pixels.size());
    for (std::size_t i = 0; i < pixels.size(); ++i) {
      auto p = lblSlice.getSlicePixels()[i];
      REQUIRE(p == pixels[i]);
      // slice pixels not transparent
      REQUIRE(qAlpha(lblSlice.getImage().pixel(p)) == 255);
    }

    // set reverse diagonal slice
    pixels = {{9, 4}, {8, 4}, {7, 4}, {6, 3}, {5, 3},
              {4, 2}, {3, 2}, {2, 1}, {1, 1}, {0, 0}};
    lblSlice.setSlice({9, 4}, {0, 0});
    REQUIRE(lblSlice.getSlicePixels().size() == pixels.size());
    for (std::size_t i = 0; i < pixels.size(); ++i) {
      auto p = lblSlice.getSlicePixels()[i];
      REQUIRE(p == pixels[i]);
      // slice pixels not transparent
      REQUIRE(qAlpha(lblSlice.getImage().pixel(p)) == 255);
    }

    // set single pixel slice
    lblSlice.setSlice({5, 3}, {5, 3});
    REQUIRE(lblSlice.getSlicePixels().size() == 1);
    REQUIRE(lblSlice.getSlicePixels()[0] == QPoint(5, 3));
    REQUIRE(qAlpha(lblSlice.getImage().pixel(5, 3)) == 255);
  }
}

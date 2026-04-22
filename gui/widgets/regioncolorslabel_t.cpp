#include "catch_wrapper.hpp"
#include "qt_test_utils.hpp"
#include "regioncolorslabel.hpp"
#include "sme/utils.hpp"
#include <QColor>
#include <QImage>
#include <QSizePolicy>
#include <algorithm>

using namespace sme::test;

namespace {

QImage render(const QWidget &widget) {
  QImage image(widget.size(), QImage::Format_ARGB32);
  image.fill(Qt::transparent);
  const_cast<QWidget &>(widget).render(&image);
  return image;
}

int regionCenterX(const QRect &rect, int region, int nRegions) {
  return rect.left() + static_cast<int>(static_cast<long long>(2 * region + 1) *
                                        rect.width() / (2 * nRegions));
}

QRect regionRect(const QRect &rect, int region, int nRegions) {
  const int x0 = rect.left() + static_cast<int>(static_cast<long long>(region) *
                                                rect.width() / nRegions);
  const int x1 =
      rect.left() + static_cast<int>(static_cast<long long>(region + 1) *
                                     rect.width() / nRegions);
  return {x0, rect.top(), std::max(1, x1 - x0), rect.height()};
}

bool containsTextPixels(const QImage &image, const QRect &cell,
                        QRgb backgroundColor) {
  const QRect textArea{
      QRect{cell.center().x() - 10, cell.top() + 2, 20, cell.height() - 4}
          .intersected(cell.adjusted(2, 2, -2, -2))};
  for (int y = textArea.top(); y <= textArea.bottom(); ++y) {
    for (int x = textArea.left(); x <= textArea.right(); ++x) {
      if (QColor(image.pixel(x, y)).rgb() != backgroundColor) {
        return true;
      }
    }
  }
  return false;
}

} // namespace

TEST_CASE("RegionColorsLabel",
          "[gui/widgets/regioncolorslabel][gui/widgets][gui][features]") {
  RegionColorsLabel label;

  REQUIRE(label.getNumberOfRegions() == 0);
  REQUIRE(label.pixmap().isNull());
  REQUIRE(label.sizePolicy().horizontalPolicy() == QSizePolicy::Expanding);
  REQUIRE(label.sizePolicy().verticalPolicy() == QSizePolicy::Fixed);
  REQUIRE(label.minimumHeight() == 24);
  REQUIRE(label.maximumHeight() == 24);
  REQUIRE(label.frameShape() == QFrame::NoFrame);

  label.resize(300, 24);
  label.setNumberOfRegions(3);
  label.show();
  waitFor(&label);

  REQUIRE(label.getNumberOfRegions() == 3);
  REQUIRE(label.pixmap().isNull());

  const auto image = render(label);
  const auto rect = label.rect();
  const int y = rect.top();
  sme::common::indexedColors indexedColors;
  REQUIRE(QColor(image.pixel(rect.left(), rect.top())).rgb() ==
          indexedColors[0].rgb());
  REQUIRE(QColor(image.pixel(regionCenterX(rect, 0, 3), y)).rgb() ==
          indexedColors[0].rgb());
  REQUIRE(QColor(image.pixel(regionCenterX(rect, 1, 3), y)).rgb() ==
          indexedColors[1].rgb());
  REQUIRE(QColor(image.pixel(regionCenterX(rect, 2, 3), y)).rgb() ==
          indexedColors[2].rgb());

  REQUIRE(containsTextPixels(image, regionRect(rect, 0, 3),
                             indexedColors[0].rgb()));
  REQUIRE(containsTextPixels(image, regionRect(rect, 1, 3),
                             indexedColors[1].rgb()));
  REQUIRE(containsTextPixels(image, regionRect(rect, 2, 3),
                             indexedColors[2].rgb()));

  label.setNumberOfRegions(0);
  REQUIRE(label.getNumberOfRegions() == 0);
  REQUIRE(label.pixmap().isNull());
}

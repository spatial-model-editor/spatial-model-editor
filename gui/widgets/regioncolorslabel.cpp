#include "regioncolorslabel.hpp"
#include "sme/utils.hpp"
#include <QPainter>
#include <algorithm>
#include <limits>

namespace {

constexpr int fixedHeight{24};
constexpr int preferredWidth{160};

QColor textColorForBackground(const QColor &background) {
  const auto luminance = 0.2126 * background.redF() +
                         0.7152 * background.greenF() +
                         0.0722 * background.blueF();
  return luminance > 0.55 ? QColor{0, 0, 0} : QColor{255, 255, 255};
}

} // namespace

RegionColorsLabel::RegionColorsLabel(QWidget *parent) : QLabel(parent) {
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  setMinimumHeight(fixedHeight);
  setMaximumHeight(fixedHeight);
  setFrameShape(QFrame::NoFrame);
  setText({});
}

void RegionColorsLabel::setNumberOfRegions(std::size_t numRegions) {
  if (nRegions == numRegions) {
    return;
  }
  nRegions = numRegions;
  update();
}

std::size_t RegionColorsLabel::getNumberOfRegions() const { return nRegions; }

QSize RegionColorsLabel::sizeHint() const {
  return {preferredWidth, fixedHeight};
}

QSize RegionColorsLabel::minimumSizeHint() const { return {0, fixedHeight}; }

void RegionColorsLabel::paintEvent(QPaintEvent *event) {
  QLabel::paintEvent(event);
  if (!isEnabled() || nRegions == 0) {
    return;
  }

  const QRect regionRect{rect()};
  if (regionRect.width() <= 0 || regionRect.height() <= 0) {
    return;
  }

  const auto nRegionsInt{static_cast<int>(std::min(
      nRegions, static_cast<std::size_t>(std::numeric_limits<int>::max())))};
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, false);
  painter.setRenderHint(QPainter::SmoothPixmapTransform, false);

  sme::common::indexedColors indexedColors;
  for (int region = 0; region < nRegionsInt; ++region) {
    const int x0 =
        regionRect.left() + static_cast<int>(static_cast<long long>(region) *
                                             regionRect.width() / nRegionsInt);
    const int x1 = regionRect.left() +
                   static_cast<int>(static_cast<long long>(region + 1) *
                                    regionRect.width() / nRegionsInt);
    const QRect rect{x0, regionRect.top(), std::max(1, x1 - x0),
                     regionRect.height()};
    painter.fillRect(rect, indexedColors[static_cast<std::size_t>(region)]);
  }

  painter.setPen(QColor{0, 0, 0, 120});
  for (int region = 1; region < nRegionsInt; ++region) {
    const int x =
        regionRect.left() + static_cast<int>(static_cast<long long>(region) *
                                             regionRect.width() / nRegionsInt);
    painter.drawLine(x, regionRect.top(), x, regionRect.bottom());
  }

  const int regionWidth{std::max(1, regionRect.width() / nRegionsInt)};
  if (regionRect.height() < 14 || regionWidth < 14) {
    return;
  }

  auto font = this->font();
  font.setBold(true);
  font.setPixelSize(
      std::max(8, std::min(regionRect.height() - 4, regionWidth / 2)));
  painter.setFont(font);
  painter.setRenderHint(QPainter::TextAntialiasing, true);
  for (int region = 0; region < nRegionsInt; ++region) {
    const int x0 =
        regionRect.left() + static_cast<int>(static_cast<long long>(region) *
                                             regionRect.width() / nRegionsInt);
    const int x1 = regionRect.left() +
                   static_cast<int>(static_cast<long long>(region + 1) *
                                    regionRect.width() / nRegionsInt);
    const QRect rect{x0, regionRect.top(), std::max(1, x1 - x0),
                     regionRect.height()};
    const auto &background{indexedColors[static_cast<std::size_t>(region)]};
    painter.setPen(textColorForBackground(background));
    painter.drawText(rect, Qt::AlignCenter, QString::number(region + 1));
  }
}

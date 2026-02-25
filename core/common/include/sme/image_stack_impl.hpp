#pragma once

#include <QImage>
#include <QList>
#include <QRgb>
#include <vector>

namespace sme::common {

/**
 * @brief Build a merged color table containing colors used across all images.
 *
 * Used when converting multiple slices to indexed format so each slice shares a
 * consistent palette.
 */
QList<QRgb> getCombinedColorTable(const std::vector<QImage> &images);

} // namespace sme::common

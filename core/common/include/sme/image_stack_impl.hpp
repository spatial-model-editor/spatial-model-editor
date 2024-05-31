#pragma once

#include <QImage>
#include <QList>
#include <QRgb>
#include <vector>

namespace sme::common {

QList<QRgb> getCombinedColorTable(const std::vector<QImage> &images);

} // namespace sme::common

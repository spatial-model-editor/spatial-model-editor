#include "sme/image_stack_impl.hpp"
#include "sme/image_stack.hpp"
#include "sme/logger.hpp"

namespace sme::common {

QList<QRgb> getCombinedColorTable(const std::vector<QImage> &images) {
  // construct a colorTable that contains all colors from all images in vector
  QList<QRgb> colorTable{};
  static constexpr int maxNumColors{256};
  for (auto &image : images) {
    auto imageColorTable =
        image
            .convertToFormat(ImageStack::indexedImageFormat,
                             ImageStack::indexedImageConversionFlags)
            .colorTable();
    for (auto color : imageColorTable) {
      if (!colorTable.contains(color)) {
        if (colorTable.size() == maxNumColors) {
          SPDLOG_WARN(
              "Image contains more than {} colors, using first {} colors",
              maxNumColors, maxNumColors);
          return colorTable;
        }
        colorTable.push_back(color);
      }
    }
  }
  return colorTable;
}

} // namespace sme::common

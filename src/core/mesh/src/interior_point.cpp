#include "interior_point.hpp"
#include "logger.hpp"
#include "mesh_utils.hpp"
#include <algorithm>
#include <opencv2/imgproc.hpp>

namespace mesh {

static std::vector<QPointF> getInnerPoints(const cv::Mat &mask) {
  std::vector<QPointF> interiorPoints;
  cv::Mat label(mask.size(), CV_16U);
  constexpr int connectivity{8};
  int nLabels{cv::connectedComponents(mask, label, connectivity, CV_16U,
                                      cv::CCL_DEFAULT)};
  SPDLOG_TRACE("{} blobs", nLabels - 1);
  // skip label 0: background
  for (int i = 1; i < nLabels; ++i) {
    cv::Mat blob{label == i};
    // crop to region of interest: roi is a reference to a subset of the pixels in blob
    auto rect{cv::boundingRect(blob)};
    cv::Mat roi(blob, rect);
    // get offset to ROI
    cv::Size size;
    cv::Point offset;
    roi.locateROI(size, offset);
    SPDLOG_TRACE("Blob {}:", i);
    SPDLOG_TRACE("ROI size: ({},{})", roi.cols, roi.rows);
    SPDLOG_TRACE("ROI offset: ({},{})", offset.x, offset.y);
    // nearest-neighbour erosion kernel
    auto kernel{cv::getStructuringElement(cv::MorphShapes::MORPH_CROSS, {3,3})};
    cv::Point inner{};
    auto p{getNonZeroPixel(roi)};
    while (p.has_value()) {
      inner = p.value();
      SPDLOG_TRACE("  - ({},{})", inner.x, inner.y);
      cv::erode(roi, roi, kernel, {-1, -1}, 1, cv::BORDER_CONSTANT, 0);
      if (rect.width > 1 && rect.height > 1) {
        // shrink ROI by a pixel in all directions
        rect.x += 1;
        rect.y += 1;
        rect.width -= 2;
        rect.height -= 2;
        roi = cv::Mat(blob, rect);
        p = getNonZeroPixel(roi);
        if(p.has_value()){
          ++offset.x;
          ++offset.y;
        }
      } else {
        p = {};
      }
    }
    interiorPoints.push_back(
        {static_cast<double>(inner.x + offset.x) + 0.5,
         static_cast<double>(mask.rows - inner.y - offset.y) - 0.5});
    SPDLOG_TRACE("  - ({},{})", interiorPoints.back().x(),
                 interiorPoints.back().y());
  }
  return interiorPoints;
}

std::vector<std::vector<QPointF>>
getInteriorPoints(const QImage &img, const std::vector<QRgb> &cols) {
  std::vector<std::vector<QPointF>> interiorPoints;
  for (auto col : cols) {
    interiorPoints.push_back(getInnerPoints(makeBinaryMask(img, col)));
  }
  return interiorPoints;
}

} // namespace mesh

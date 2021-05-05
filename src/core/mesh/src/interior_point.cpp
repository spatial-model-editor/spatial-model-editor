#include "interior_point.hpp"
#include "logger.hpp"
#include "mesh_utils.hpp"
#include <algorithm>
#include <opencv2/imgproc.hpp>
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_TRACE
#include <QImage>
#endif

namespace sme::mesh {

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_TRACE
static void writeDebuggingImageOutput(const cv::Mat &mask, const cv::Mat &blob,
                                      const cv::Rect &rect,
                                      const cv::Point &point,
                                      const QString &name) {
  const int Y{blob.rows};
  const int X{blob.cols};
  QImage img(X, Y, QImage::Format_RGB32);
  constexpr QRgb bg{qRgb(255, 255, 255)};
  constexpr QRgb mk{qRgb(177, 177, 177)};
  constexpr QRgb fg{qRgb(0, 0, 0)};
  constexpr QRgb l{qRgb(0, 255, 0)};
  constexpr QRgb p{qRgb(255, 0, 0)};
  img.fill(bg);
  // mask
  for (int y = 0; y < Y; ++y) {
    const uchar *row{mask.ptr(y)};
    for (int x = 0; x < X; ++x) {
      if (row[x] > 0) {
        img.setPixel(x, y, mk);
      }
    }
  }
  // ROI box
  for (int y : {rect.y, rect.y + rect.height - 1}) {
    for (int x = rect.x; x < rect.x + rect.width; ++x) {
      img.setPixel(x, y, l);
    }
  }
  for (int x : {rect.x, rect.x + rect.width - 1}) {
    for (int y = rect.y; y < rect.y + rect.height; ++y) {
      img.setPixel(x, y, l);
    }
  }
  // blob
  for (int y = 0; y < Y; ++y) {
    const uchar *row{blob.ptr(y)};
    for (int x = 0; x < X; ++x) {
      if (row[x] > 0) {
        img.setPixel(x, y, fg);
      }
    }
  }
  // interior point
  img.setPixel(point.x, point.y, p);
  img.save(name);
}
#endif

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
    // crop to region of interest: roi is a reference to a subset of the pixels
    // in blob
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
    auto kernel{
        cv::getStructuringElement(cv::MorphShapes::MORPH_CROSS, {3, 3})};
    cv::Point inner{};
    auto p{getNonZeroPixel(roi)};
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_TRACE
    int j{10};
    writeDebuggingImageOutput(mask, blob, rect, p.value() + offset,
                              QString("blob%1_%2").arg(i).arg(j++));
#endif
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
        if (p.has_value()) {
          ++offset.x;
          ++offset.y;
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_TRACE
          writeDebuggingImageOutput(mask, blob, rect, p.value() + offset,
                                    QString("blob%1_%2.png").arg(i).arg(j++));
#endif
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

} // namespace sme

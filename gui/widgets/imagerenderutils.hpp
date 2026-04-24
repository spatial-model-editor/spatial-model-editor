#pragma once

#include "sme/voxel.hpp"
#include <QImage>
#include <QPixmap>
#include <QString>

namespace sme::gui {

struct ImageRenderOptions {
  Qt::AspectRatioMode aspectRatioMode{Qt::KeepAspectRatio};
  Qt::TransformationMode transformationMode{Qt::FastTransformation};
  bool drawGrid{false};
  bool drawScale{false};
  bool flipYAxis{false};
  int verticalIndicatorSourceX{-1};
  int tickLength{10};
  common::VoxelF physicalOrigin{0.0, 0.0, 0.0};
  common::VolumeF physicalSize{1.0, 1.0, 1.0};
  QString lengthUnits{};
};

struct RenderedImage {
  QPixmap pixmap{};
  QSize pixmapImageSize{};
  QPoint offset{0, 0};
};

[[nodiscard]] RenderedImage
renderImageWithOverlays(const QImage &srcImage, const QSize &displaySize,
                        const ImageRenderOptions &options);

} // namespace sme::gui

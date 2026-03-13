#include "gpumemoryusagebarwidget.hpp"
#include "sme/utils.hpp"
#include <QPainter>
#include <algorithm>
#include <cmath>
#include <utility>

GpuMemoryUsageBarWidget::GpuMemoryUsageBarWidget(QWidget *parent)
#ifdef SME_WITH_CUDA
    : GpuMemoryUsageBarWidget(sme::simulate::getCudaMemoryInfo, parent){}
#else
    : GpuMemoryUsageBarWidget(
          []() -> std::optional<sme::simulate::CudaMemoryInfo> {
            return std::nullopt;
          },
          parent) {
}
#endif

      GpuMemoryUsageBarWidget::GpuMemoryUsageBarWidget(
          CudaMemoryInfoProvider cudaMemoryInfoProvider, QWidget * parent,
          int updateIntervalMs)
    : QWidget(parent),
      cudaMemoryInfoProvider{std::move(cudaMemoryInfoProvider)} {
  setObjectName("statusBarGpuMemoryUsageBar");
  setMinimumWidth(140);
  setFixedHeight(14);
  updateTimer.setTimerType(Qt::TimerType::VeryCoarseTimer);
  setUpdateIntervalMs(updateIntervalMs);
  connect(&updateTimer, &QTimer::timeout, this,
          &GpuMemoryUsageBarWidget::refresh);
  updateTimer.start();
  refresh();
}

void GpuMemoryUsageBarWidget::setUpdateIntervalMs(int updateIntervalMs) {
  updateTimer.setInterval(updateIntervalMs);
}

void GpuMemoryUsageBarWidget::clearMemoryUsage() {
  usedBytes = 0;
  availableBytes = 0;
  totalBytes = 0;
  setToolTip(QString{});
  hide();
  update();
}

void GpuMemoryUsageBarWidget::refresh() {
  const auto totalMemoryInfo{cudaMemoryInfoProvider()};
  if (!totalMemoryInfo.has_value() || totalMemoryInfo->totalBytes == 0) {
    clearMemoryUsage();
    return;
  }

  const std::size_t totalBytes{totalMemoryInfo->totalBytes};
  const std::size_t totalAvailableBytes{
      std::min(totalMemoryInfo->availableBytes, totalBytes)};
  const std::size_t totalUsedBytes{totalBytes - totalAvailableBytes};
  setMemoryUsage(totalUsedBytes, totalAvailableBytes, totalBytes);
  show();
}

void GpuMemoryUsageBarWidget::setMemoryUsage(std::size_t used,
                                             std::size_t available,
                                             std::size_t total) {
  usedBytes = used;
  availableBytes = available;
  totalBytes = total;
  setToolTip(QString("GPU RAM used: %1\n"
                     "GPU RAM total available: %2\n"
                     "GPU RAM total: %3")
                 .arg(sme::common::formatMemoryBytes(usedBytes),
                      sme::common::formatMemoryBytes(availableBytes),
                      sme::common::formatMemoryBytes(totalBytes)));
  update();
}

void GpuMemoryUsageBarWidget::paintEvent(QPaintEvent *event) {
  Q_UNUSED(event);
  QPainter painter(this);
  const QRect outerRect = rect().adjusted(0, 0, -1, -1);
  painter.setPen(palette().color(QPalette::ColorRole::Mid));
  painter.setBrush(Qt::NoBrush);
  painter.drawRect(outerRect);
  const QRect innerRect = outerRect.adjusted(1, 1, -1, -1);
  if (innerRect.width() <= 0 || innerRect.height() <= 0 || totalBytes == 0) {
    return;
  }

  const auto totalAsDouble = static_cast<double>(totalBytes);
  const int innerWidth = innerRect.width();
  const int usedWidth = static_cast<int>(
      std::floor(static_cast<double>(usedBytes) *
                 static_cast<double>(innerWidth) / totalAsDouble));
  const int freeWidth = std::max(0, innerWidth - usedWidth);

  int x = innerRect.left();
  if (usedWidth > 0) {
    painter.fillRect(QRect(x, innerRect.top(), usedWidth, innerRect.height()),
                     QColor("#1F4E79"));
    x += usedWidth;
  }
  if (freeWidth > 0) {
    painter.fillRect(QRect(x, innerRect.top(), freeWidth, innerRect.height()),
                     QColor("#FFFFFF"));
  }
}

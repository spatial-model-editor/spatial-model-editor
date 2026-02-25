#include "memoryusagebarwidget.hpp"
#include "sme/utils.hpp"
#include <QPainter>
#include <algorithm>
#include <cmath>
#include <utility>

MemoryUsageBarWidget::MemoryUsageBarWidget(QWidget *parent)
    : MemoryUsageBarWidget(sme::common::getSystemMemoryInfo,
                           sme::common::getCurrentProcessMemoryUsageBytes,
                           parent) {}

MemoryUsageBarWidget::MemoryUsageBarWidget(
    SystemMemoryInfoProvider systemMemoryInfoProvider,
    ProcessMemoryUsageProvider processMemoryUsageProvider, QWidget *parent,
    int updateIntervalMs)
    : QWidget(parent),
      systemMemoryInfoProvider{std::move(systemMemoryInfoProvider)},
      processMemoryUsageProvider{std::move(processMemoryUsageProvider)} {
  setObjectName("statusBarMemoryUsageBar");
  setMinimumWidth(140);
  setFixedHeight(14);
  updateTimer.setTimerType(Qt::TimerType::VeryCoarseTimer);
  setUpdateIntervalMs(updateIntervalMs);
  connect(&updateTimer, &QTimer::timeout, this, &MemoryUsageBarWidget::refresh);
  updateTimer.start();
  refresh();
}

void MemoryUsageBarWidget::setUpdateIntervalMs(int updateIntervalMs) {
  updateTimer.setInterval(updateIntervalMs);
}

void MemoryUsageBarWidget::refresh() {
  const auto totalMemoryInfo{systemMemoryInfoProvider()};
  const auto processMemoryUsage{processMemoryUsageProvider()};
  if (!totalMemoryInfo.has_value() || !processMemoryUsage.has_value() ||
      totalMemoryInfo->totalPhysicalBytes == 0) {
    hide();
    return;
  }

  const std::size_t totalBytes{totalMemoryInfo->totalPhysicalBytes};
  const std::size_t totalAvailableBytes{
      std::min(totalMemoryInfo->availablePhysicalBytes, totalBytes)};
  const std::size_t totalUsedBytes{totalBytes - totalAvailableBytes};
  const std::size_t smeUsedBytes{
      std::min(processMemoryUsage.value(), totalUsedBytes)};
  const std::size_t systemUsedBytes{totalUsedBytes - smeUsedBytes};
  setMemoryUsage(smeUsedBytes, systemUsedBytes, totalAvailableBytes,
                 totalBytes);
  show();
}

void MemoryUsageBarWidget::setMemoryUsage(std::size_t smeUsed,
                                          std::size_t systemUsed,
                                          std::size_t totalAvailable,
                                          std::size_t total) {
  smeUsedBytes = smeUsed;
  systemUsedBytes = systemUsed;
  totalAvailableBytes = totalAvailable;
  totalBytes = total;
  setToolTip(QString("RAM used by SME: %1\n"
                     "RAM used by system: %2\n"
                     "RAM total available: %3")
                 .arg(sme::common::formatMemoryBytes(smeUsedBytes),
                      sme::common::formatMemoryBytes(systemUsedBytes),
                      sme::common::formatMemoryBytes(totalAvailableBytes)));
  update();
}

void MemoryUsageBarWidget::paintEvent(QPaintEvent *event) {
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
  const int smeWidth = static_cast<int>(
      std::floor(static_cast<double>(smeUsedBytes) *
                 static_cast<double>(innerWidth) / totalAsDouble));
  const int systemWidth = static_cast<int>(
      std::floor(static_cast<double>(systemUsedBytes) *
                 static_cast<double>(innerWidth) / totalAsDouble));
  const int availableWidth = std::max(0, innerWidth - smeWidth - systemWidth);

  int x = innerRect.left();
  if (smeWidth > 0) {
    painter.fillRect(QRect(x, innerRect.top(), smeWidth, innerRect.height()),
                     QColor("#4A4A4A"));
    x += smeWidth;
  }
  if (systemWidth > 0) {
    painter.fillRect(QRect(x, innerRect.top(), systemWidth, innerRect.height()),
                     QColor("#BDBDBD"));
    x += systemWidth;
  }
  if (availableWidth > 0) {
    painter.fillRect(
        QRect(x, innerRect.top(), availableWidth, innerRect.height()),
        QColor("#FFFFFF"));
  }
}

#pragma once

#include "sme/system_memory.hpp"
#include <QTimer>
#include <QWidget>
#include <cstddef>
#include <functional>
#include <optional>

class MemoryUsageBarWidget : public QWidget {
public:
  using SystemMemoryInfoProvider =
      std::function<std::optional<sme::common::SystemMemoryInfo>()>;
  using ProcessMemoryUsageProvider =
      std::function<std::optional<std::size_t>()>;

  explicit MemoryUsageBarWidget(QWidget *parent = nullptr);

  MemoryUsageBarWidget(SystemMemoryInfoProvider systemMemoryInfoProvider,
                       ProcessMemoryUsageProvider processMemoryUsageProvider,
                       QWidget *parent = nullptr, int updateIntervalMs = 2000);

  void refresh();
  void setUpdateIntervalMs(int updateIntervalMs);

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  SystemMemoryInfoProvider systemMemoryInfoProvider;
  ProcessMemoryUsageProvider processMemoryUsageProvider;
  QTimer updateTimer;
  std::size_t smeUsedBytes{};
  std::size_t systemUsedBytes{};
  std::size_t totalAvailableBytes{};
  std::size_t totalBytes{};

  void setMemoryUsage(std::size_t smeUsed, std::size_t systemUsed,
                      std::size_t totalAvailable, std::size_t total);
};

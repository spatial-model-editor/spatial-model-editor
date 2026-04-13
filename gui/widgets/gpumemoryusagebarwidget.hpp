#pragma once

#include "sme/cuda_stubs.hpp"
#include <QTimer>
#include <QWidget>
#include <cstddef>
#include <functional>
#include <optional>

class GpuMemoryUsageBarWidget : public QWidget {
public:
  using CudaMemoryInfoProvider =
      std::function<std::optional<sme::simulate::CudaMemoryInfo>()>;

  explicit GpuMemoryUsageBarWidget(QWidget *parent = nullptr);

  GpuMemoryUsageBarWidget(CudaMemoryInfoProvider cudaMemoryInfoProvider,
                          QWidget *parent = nullptr,
                          int updateIntervalMs = 2000);

  void refresh();
  void setUpdateIntervalMs(int updateIntervalMs);

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  CudaMemoryInfoProvider cudaMemoryInfoProvider;
  QTimer updateTimer;
  std::size_t usedBytes{};
  std::size_t availableBytes{};
  std::size_t totalBytes{};

  void clearMemoryUsage();
  void setMemoryUsage(std::size_t used, std::size_t available,
                      std::size_t total);
};

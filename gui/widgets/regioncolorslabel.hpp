#pragma once

#include <QLabel>
#include <QSize>
#include <cstddef>

class RegionColorsLabel : public QLabel {
  Q_OBJECT

public:
  explicit RegionColorsLabel(QWidget *parent = nullptr);

  void setNumberOfRegions(std::size_t nRegions);
  [[nodiscard]] std::size_t getNumberOfRegions() const;
  [[nodiscard]] QSize sizeHint() const override;
  [[nodiscard]] QSize minimumSizeHint() const override;

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  std::size_t nRegions{};
};

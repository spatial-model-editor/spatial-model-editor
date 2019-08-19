// a set of default colours for display purposes

#pragma once

#include <vector>

#include <QColor>

namespace colours {

class indexedColours {
 private:
  const std::vector<QColor> colours{
      {230, 25, 75},  {60, 180, 75},   {255, 225, 25}, {0, 130, 200},
      {245, 130, 48}, {145, 30, 180},  {70, 240, 240}, {240, 50, 230},
      {210, 245, 60}, {250, 190, 190}, {0, 128, 128},  {230, 190, 255},
      {170, 110, 40}, {255, 250, 200}, {128, 0, 0},    {170, 255, 195},
      {128, 128, 0},  {255, 215, 180}, {0, 0, 128},    {128, 128, 128}};

 public:
  const QColor &operator[](std::size_t i) const;
};

}  // namespace colours

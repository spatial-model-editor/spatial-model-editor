// Wrapper around libTIFF
//  - writes field concentration as 16-bit grayscale tiff

#pragma once

#include <string>
#include <vector>

#include "geometry.hpp"

namespace utils {

double writeTIFF(const std::string& filename, const geometry::Field& field,
                 double pixelWidth = 1.0);

}

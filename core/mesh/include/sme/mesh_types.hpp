#pragma once

#include <QPointF>
#include <array>

namespace sme::mesh {

using QTriangleF = std::array<QPointF, 3>;
using TriangulateTriangleIndex = std::array<std::size_t, 3>;

} // namespace sme::mesh

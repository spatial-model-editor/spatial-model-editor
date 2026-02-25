#pragma once

#include <QPointF>
#include <array>

namespace sme::mesh {

/**
 * @brief Triangle represented by 3 points in 2D.
 */
using QTriangleF = std::array<QPointF, 3>;
/**
 * @brief Triangle vertex indices into a mesh vertex array.
 */
using TriangulateTriangleIndex = std::array<std::size_t, 3>;

} // namespace sme::mesh

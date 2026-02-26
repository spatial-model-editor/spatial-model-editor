// copy of Regular_triangulation_vertex_base_3 from CGAL with an id

#pragma once

#include <CGAL/Regular_triangulation_vertex_base_3.h>

namespace sme::mesh {

/**
 * @brief ``CGAL::Regular_triangulation_vertex_base_3`` extension with integer
 * id storage.
 *
 * Provides stable id assignment for vertices used during tetrahedralization.
 */
template <typename GT,
          typename Vb = CGAL::Regular_triangulation_vertex_base_3<GT>>
class CGAL_Regular_triangulation_vertex_base_3_with_id : public Vb {
  int _id{};

public:
  using Cell_handle = typename Vb::Cell_handle;
  using Geom_traits = typename Vb::Geom_traits;
  using Point = typename Vb::Point;

  /**
   * @brief Rebind this vertex base type for a different triangulation data
   * structure.
   */
  template <typename TDS2> struct Rebind_TDS {
    using Vb2 = typename Vb::template Rebind_TDS<TDS2>::Other;
    using Other = CGAL_Regular_triangulation_vertex_base_3_with_id<GT, Vb2>;
  };

  /**
   * @brief Default constructor.
   */
  CGAL_Regular_triangulation_vertex_base_3_with_id() : Vb() {}

  /**
   * @brief Construct from point.
   */
  explicit CGAL_Regular_triangulation_vertex_base_3_with_id(const Point &p)
      : Vb(p) {}

  /**
   * @brief Construct from point and cell handle.
   */
  CGAL_Regular_triangulation_vertex_base_3_with_id(const Point &p,
                                                   Cell_handle c)
      : Vb(p, c) {}

  /**
   * @brief Construct from cell handle.
   */
  explicit CGAL_Regular_triangulation_vertex_base_3_with_id(Cell_handle c)
      : Vb(c) {}

  /**
   * @brief Read stored vertex id.
   */
  [[nodiscard]] int id() const { return _id; }
  /**
   * @brief Mutable stored vertex id.
   */
  int &id() { return _id; }
};

} // namespace sme::mesh

// copy of Regular_triangulation_vertex_base_3 from CGAL with an id

#pragma once

#include <CGAL/Regular_triangulation_vertex_base_3.h>

namespace sme::mesh {

template <typename GT,
          typename Vb = CGAL::Regular_triangulation_vertex_base_3<GT>>
class CGAL_Regular_triangulation_vertex_base_3_with_id : public Vb {
  int _id{};

public:
  using Cell_handle = typename Vb::Cell_handle;
  using Geom_traits = typename Vb::Geom_traits;
  using Point = typename Vb::Point;

  template <typename TDS2> struct Rebind_TDS {
    using Vb2 = typename Vb::template Rebind_TDS<TDS2>::Other;
    using Other = CGAL_Regular_triangulation_vertex_base_3_with_id<GT, Vb2>;
  };

  CGAL_Regular_triangulation_vertex_base_3_with_id() : Vb() {}

  explicit CGAL_Regular_triangulation_vertex_base_3_with_id(const Point &p)
      : Vb(p) {}

  CGAL_Regular_triangulation_vertex_base_3_with_id(const Point &p,
                                                   Cell_handle c)
      : Vb(p, c) {}

  explicit CGAL_Regular_triangulation_vertex_base_3_with_id(Cell_handle c)
      : Vb(c) {}

  [[nodiscard]] int id() const { return _id; }
  int &id() { return _id; }
};

} // namespace sme::mesh

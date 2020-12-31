target_sources(
  core
  PRIVATE boundary.cpp
          contour_map.cpp
          line_simplifier.cpp
          mesh.cpp
          pixel_corner_iterator.cpp
          triangulate.cpp)
target_sources(
  core_tests
  PUBLIC boundary.cpp
         boundary_t.cpp
         contour_map.cpp
         contour_map_t.cpp
         line_simplifier.cpp
         line_simplifier_t.cpp
         mesh_t.cpp
         pixel_corner_iterator.cpp
         pixel_corner_iterator_t.cpp
         triangulate.cpp
         triangulate_t.cpp)

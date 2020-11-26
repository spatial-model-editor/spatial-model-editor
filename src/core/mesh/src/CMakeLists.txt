target_sources(
  core
  PRIVATE boundary.cpp
          contour_map.cpp
          contours.cpp
          image_boundaries.cpp
          line_simplifier.cpp
          mesh.cpp
          triangulate.cpp)
target_sources(
  core_tests
  PUBLIC boundary.cpp
         boundary_t.cpp
         contour_map.cpp
         contour_map_t.cpp
         contours.cpp
         contours_t.cpp
         image_boundaries.cpp
         image_boundaries_t.cpp
         line_simplifier.cpp
         line_simplifier_t.cpp
         mesh_t.cpp
         triangulate.cpp
         triangulate_t.cpp)

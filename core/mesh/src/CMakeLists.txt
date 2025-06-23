target_sources(
  core
  PRIVATE boundaries.cpp
          boundary.cpp
          contour_map.cpp
          interior_point.cpp
          line_simplifier.cpp
          mesh2d.cpp
          mesh3d.cpp
          mesh_utils.cpp
          pixel_corner_iterator.cpp
          polyline_simplifier.cpp
          triangulate.cpp
          triangulate_common.cpp)
if(BUILD_TESTING)
  target_sources(
    core_tests
    PUBLIC boundaries_t.cpp
           boundary_t.cpp
           contour_map_t.cpp
           interior_point_t.cpp
           line_simplifier_t.cpp
           mesh2d_t.cpp
           mesh3d_t.cpp
           mesh_utils_t.cpp
           polyline_simplifier_t.cpp
           pixel_corner_iterator_t.cpp
           triangulate_t.cpp)
endif()
if(SME_BUILD_BENCHMARKS)
  target_sources(
    bench
    PUBLIC boundaries_bench.cpp
           interior_point_bench.cpp
           line_simplifier_bench.cpp
           mesh2d_bench.cpp
           triangulate_bench.cpp)
endif()

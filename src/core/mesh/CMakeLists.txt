target_sources(core
               PRIVATE
               boundary.cpp
               boundary_pixels.cpp
               image_boundaries.cpp
               line_simplifier.cpp 
               mesh.cpp
               triangulate.cpp)

target_include_directories(core PUBLIC .)

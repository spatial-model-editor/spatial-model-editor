target_sources(core
               PRIVATE
               geometry_sampled_field.cpp
               math.cpp
               sbml.cpp
               xml_annotation.cpp)

target_include_directories(core PUBLIC .)


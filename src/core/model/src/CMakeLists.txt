target_sources(
  core
  PRIVATE geometry.cpp
          geometry_parametric.cpp
          geometry_sampled_field.cpp
          id.cpp
          math.cpp
          model.cpp
          model_compartments.cpp
          model_functions.cpp
          model_geometry.cpp
          model_math.cpp
          model_membranes.cpp
          model_membranes_util.cpp
          model_parameters.cpp
          model_reactions.cpp
          model_species.cpp
          model_units.cpp
          sbml_utils.cpp
          validation.cpp
          xml_annotation.cpp)

target_sources(
  core_tests
  PUBLIC geometry_t.cpp
         model_t.cpp
         model_functions_t.cpp
         model_math_t.cpp
         model_membranes_t.cpp
         model_membranes_util_t.cpp
         model_parameters_t.cpp
         model_units_t.cpp)

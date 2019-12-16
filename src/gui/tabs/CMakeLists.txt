target_sources(gui
               PRIVATE
               qtabfunctions.cpp
               qtabfunctions.hpp
               qtabgeometry.cpp
               qtabgeometry.hpp
               qtabreactions.cpp
               qtabreactions.hpp
               qtabsimulate.cpp
               qtabsimulate.hpp
               qtabspecies.cpp
               qtabspecies.hpp)

target_include_directories(gui PUBLIC .)

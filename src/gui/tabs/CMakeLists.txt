target_sources(gui
               PRIVATE
               tabfunctions.cpp
               tabfunctions.hpp
               tabgeometry.cpp
               tabgeometry.hpp
               tabmembranes.cpp
               tabmembranes.hpp
               tabreactions.cpp
               tabreactions.hpp
               tabsimulate.cpp
               tabsimulate.hpp
               tabspecies.cpp
               tabspecies.hpp)

target_include_directories(gui PUBLIC .)

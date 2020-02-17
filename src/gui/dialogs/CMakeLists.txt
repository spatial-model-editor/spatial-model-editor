target_sources(gui
               PRIVATE
               dialogabout.cpp
               dialogabout.hpp
               dialoganalytic.cpp
               dialoganalytic.hpp
               dialogconcentrationimage.cpp
               dialogconcentrationimage.hpp
               dialogdisplayoptions.cpp
               dialogdisplayoptions.hpp
               dialogimagesize.cpp
               dialogimagesize.hpp
               dialogimageslice.cpp
               dialogimageslice.hpp
               dialogintegratoroptions.cpp
               dialogintegratoroptions.hpp
               dialogunits.cpp
               dialogunits.hpp)

target_include_directories(gui PUBLIC .)

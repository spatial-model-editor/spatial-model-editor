add_library(qcustomplot STATIC qcustomplot.cpp qcustomplot.h)
find_package(
  Qt5
  COMPONENTS Widgets PrintSupport
  REQUIRED)
target_link_libraries(qcustomplot PUBLIC Qt5::Widgets Qt5::PrintSupport)
target_include_directories(qcustomplot SYSTEM PUBLIC .)
target_compile_options(
  qcustomplot
  PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:-Wno-deprecated
    -Wno-deprecated-declarations>)

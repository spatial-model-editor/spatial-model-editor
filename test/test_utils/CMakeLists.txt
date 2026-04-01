target_sources(
  testlib
  PUBLIC catch_wrapper.cpp
         math_test_utils.cpp
         model_test_utils.cpp)
if(SME_BUILD_GUI)
  target_sources(testlib PUBLIC qt_test_utils.cpp)
endif()
target_include_directories(testlib PUBLIC .)

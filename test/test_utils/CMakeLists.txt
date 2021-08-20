target_sources(
  testlib
  PUBLIC catch_wrapper.cpp
         math_test_utils.cpp
         model_test_utils.cpp
         qt_test_utils.cpp)
target_include_directories(testlib PUBLIC .)

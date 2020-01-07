add_library(qcustomplot STATIC qcustomplot.cpp qcustomplot.h)
target_link_libraries(qcustomplot PRIVATE Qt5::Widgets Qt5::PrintSupport)
target_include_directories(qcustomplot SYSTEM
                           PUBLIC ${PROJECT_SOURCE_DIR}/ext/qcustomplot)

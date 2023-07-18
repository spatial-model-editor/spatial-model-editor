target_sources(
        gui
        PRIVATE
        IArray.cpp
        Camera.cpp
        ObjectLoader.cpp
        ShaderProgram.cpp
        Utils.cpp
        Vector3.cpp
        Vector4.cpp
        WireframeObject.cpp)

target_include_directories(
        gui
        PUBLIC
        .)

if(BUILD_TESTING)
    target_sources(
            gui_tests
            PUBLIC )
endif()
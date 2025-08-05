
add_executable(viewer
    src/goldsrcviewerapp.cpp
    src/goldsrcviewerapp.h
    src/main.cpp
    src/renderers/opengl/openglrenderer.cpp
    src/renderers/opengl/openglrenderer.hpp
)

target_include_directories(viewer
    PRIVATE
        include
)

target_link_libraries(viewer
    PRIVATE
        common
        construct
        glm
        EnTT
        imgui
)

if(MINGW)
    target_link_options(viewer
        PUBLIC
            -static
    )
endif()


add_executable(game
    src/genmapapp.cpp
    src/genmapapp.h
    src/main.cpp
    src/renderers/opengl/openglrenderer.cpp
    src/renderers/opengl/openglrenderer.hpp
    src/console.h
    src/console.cpp
)

target_include_directories(game
    PRIVATE
        include
)

target_link_libraries(game
    PRIVATE
        common
        construct
        glm
        EnTT
)

if(MINGW)
    target_link_options(game
        PUBLIC
            -static
    )
endif()

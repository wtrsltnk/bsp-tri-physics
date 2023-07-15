cmake_minimum_required(VERSION 3.7)

include(cmake/CPM.cmake)
include(cmake/Dependencies.cmake)

project(bsp-tri-physics)

add_library(common
    README.md
    common/include/application.h
    common/include/fontmanager.h
    common/include/glad/glad.h
    common/include/ifont.h
    common/include/inputstate.h
    common/include/stb_image.h
    common/include/stb_rect_pack.h
    common/include/stb_truetype.h
    common/src/fontmanager.cpp
    common/src/glad.c
    common/src/stb_image.cpp
    common/src/stb_rect_pack.cpp
    common/src/stb_truetype.cpp
)

if(WIN32)
    find_package(OPENGL REQUIRED)

    target_sources(common
        PRIVATE
            common/src/application-win32.cpp
            common/src/glad_wgl.c
    )

    target_link_libraries(common
        PRIVATE
            ${OPENGL_LIBRARIES}
            spdlog
    )
endif()

target_include_directories(common
    PUBLIC
        common/include
)

add_library(construct
    construct/include/assetmanager.h
    construct/include/camera.h
    construct/include/engine.hpp
    construct/include/entities.hpp
    construct/include/entitycomponents.h
    construct/include/glbuffer.h
    construct/include/glshader.h
    construct/include/iassetmanager.hpp
    construct/include/iphysicsservice.hpp
    construct/include/irenderer.hpp
    construct/include/valve/bsp/hl1bspasset.h
    construct/include/valve/bsp/hl1bsptypes.h
    construct/include/valve/bsp/hl1wadasset.h
    construct/include/valve/hl1filesystem.h
    construct/include/valve/hltexture.h
    construct/include/valve/hltypes.h
    construct/include/valve/mdl/hl1mdlasset.h
    construct/include/valve/mdl/hl1mdlinstance.h
    construct/include/valve/mdl/hl1mdltypes.h
    construct/include/valve/spr/hl1sprasset.h
    construct/include/valve/spr/hl1sprtypes.h
    construct/src/assetmanager.cpp
    construct/src/camera.cpp
    construct/src/engine.cpp
    construct/src/glbuffer.cpp
    construct/src/glshader.cpp
    construct/src/physicsservice.cpp
    construct/src/valve/bsp/hl1bspasset.cpp
    construct/src/valve/bsp/hl1wadasset.cpp
    construct/src/valve/hl1filesystem.cpp
    construct/src/valve/hltexture.cpp
    construct/src/valve/mdl/hl1mdlasset.cpp
    construct/src/valve/mdl/hl1mdlinstance.cpp
    construct/src/valve/spr/hl1sprasset.cpp
    construct/src/vertexarray.cpp
)

target_include_directories(construct
    PUBLIC
        construct/include
)

target_link_libraries(construct
    PRIVATE
        glm
)

target_compile_features(construct
    PUBLIC
        cxx_std_20
        cxx_auto_type
        cxx_nullptr
        cxx_range_for
        cxx_thread_local
)

target_link_libraries(construct
    PUBLIC
        common
        glm
        spdlog
        EnTT
        BulletDynamics
        BulletCollision
        BulletSoftBody
        LinearMath
        bullet3
)

add_subdirectory(game)
add_subdirectory(viewer)

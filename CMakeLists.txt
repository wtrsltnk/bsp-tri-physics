cmake_minimum_required(VERSION 3.7)

include(cmake/CPM.cmake)

project(bsp-tri-physics)

CPMAddPackage(
    NAME spdlog
    GITHUB_REPOSITORY gabime/spdlog
    VERSION 1.7.0
)

CPMAddPackage(
    NAME glm
    GITHUB_REPOSITORY g-truc/glm
    GIT_TAG 0.9.9.7
)

CPMAddPackage(
    NAME EnTT
    VERSION 3.1.1
    GITHUB_REPOSITORY skypjack/entt
    # EnTT's CMakeLists screws with configuration options
    DOWNLOAD_ONLY True
)

if (EnTT_ADDED)
    add_library(EnTT INTERFACE)
    target_include_directories(EnTT INTERFACE ${EnTT_SOURCE_DIR}/src)
endif()

CPMAddPackage(
    NAME bullet3
    GITHUB_REPOSITORY bulletphysics/bullet3
    GIT_TAG 2.89
    OPTIONS
        "USE_DOUBLE_PRECISION Off"
        "USE_GRAPHICAL_BENCHMARK Off"
        "USE_CUSTOM_VECTOR_MATH Off"
        "USE_MSVC_INCREMENTAL_LINKING Off"
        "USE_MSVC_RUNTIME_LIBRARY_DLL On"
        "USE_GLUT Off"
        "BUILD_DEMOS Off"
        "BUILD_CPU_DEMOS Off"
        "BUILD_BULLET3 Off"
        "BUILD_BULLET2_DEMOS Off"
        "BUILD_EXTRAS Off"
        "INSTALL_EXTRA_LIBS Off"
        "BUILD_UNIT_TESTS Off"
        "INSTALL_LIBS On"
)

if (bullet3_ADDED)
    add_library(bullet3 INTERFACE)
    target_include_directories(bullet3 INTERFACE ${bullet3_SOURCE_DIR}/src)
endif()

add_library(glad
    common/include/glad/glad.h
    common/src/glad.c
)

if(WIN32)
    target_sources(glad
        PRIVATE
            common/src/glad_wgl.c
    )
endif()

target_include_directories(glad
    PUBLIC
        common/include
)

add_library(stb
    common/include/stb_image.h
    common/include/stb_rect_pack.h
    common/src/stb_image.cpp
    common/src/stb_rect_pack.cpp
)

target_include_directories(stb
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
    construct/include/inputstate.h
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
        glad
        stb
        glm
        spdlog
        EnTT
        BulletDynamics
        BulletCollision
        BulletSoftBody
        LinearMath
        bullet3
)

add_executable(bsp-tri-physics
    README.md
    game/include/application.h
    game/src/genmapapp.cpp
    game/src/genmapapp.h
    game/src/main.cpp
    game/src/renderers/opengl/openglrenderer.cpp
    game/src/renderers/opengl/openglrenderer.hpp
)

if(WIN32)
    find_package(OPENGL REQUIRED)

    target_sources(bsp-tri-physics
        PUBLIC
            game/src/application-win32.cpp
    )

    target_link_libraries(bsp-tri-physics
        PRIVATE
            ${OPENGL_LIBRARIES}
    )
endif()

target_include_directories(bsp-tri-physics
    PRIVATE
        game/include
)

target_link_libraries(bsp-tri-physics
    PRIVATE
        glad
        stb
        construct
        glm
        spdlog
        EnTT
)

if(MINGW)
    target_link_options(bsp-tri-physics
        PUBLIC
            -static
    )
endif()

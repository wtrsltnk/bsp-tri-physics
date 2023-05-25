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

add_executable(bsp-tri-physics
    include/application.h
    include/glad.c
    include/glad/glad.h
    include/glbuffer.h
    include/glshader.h
    include/iphysicsservice.hpp
    include/irenderer.hpp
    include/stb_image.h
    include/stb_rect_pack.h
    src/assetmanager.cpp
    src/assetmanager.h
    src/camera.cpp
    src/camera.h
    src/entitycomponents.h
    src/genmapapp.cpp
    src/genmapapp.h
    src/glbuffer.cpp
    src/glshader.cpp
    src/main.cpp
    src/physicsservice.cpp
    src/physicsservice.hpp
    src/renderers/opengl/openglrenderer.cpp
    src/renderers/opengl/openglrenderer.hpp
    src/stb_image.cpp
    src/stb_rect_pack.cpp
    src/valve/bsp/hl1bspasset.cpp
    src/valve/bsp/hl1bspasset.h
    src/valve/bsp/hl1bsptypes.h
    src/valve/bsp/hl1wadasset.cpp
    src/valve/bsp/hl1wadasset.h
    src/valve/hl1filesystem.cpp
    src/valve/hl1filesystem.h
    src/valve/hltexture.cpp
    src/valve/hltexture.h
    src/valve/hltypes.h
    src/valve/mdl/hl1mdlasset.cpp
    src/valve/mdl/hl1mdlasset.h
    src/valve/mdl/hl1mdlinstance.cpp
    src/valve/mdl/hl1mdlinstance.h
    src/valve/mdl/hl1mdltypes.h
    src/valve/spr/hl1sprasset.cpp
    src/valve/spr/hl1sprasset.h
    src/valve/spr/hl1sprtypes.h
    src/vertexarray.cpp
)

if(WIN32)
    find_package(OPENGL REQUIRED)

    target_sources(bsp-tri-physics
        PUBLIC
            include/application-win32.cpp
            include/glad_wgl.c
    )

    target_link_libraries(bsp-tri-physics
        PRIVATE
            ${OPENGL_LIBRARIES}
    )
endif()

target_sources(bsp-tri-physics
    PRIVATE
        include/GL/wglext.h
)

target_include_directories(bsp-tri-physics
    PRIVATE
        include
        glad
)

target_link_libraries(bsp-tri-physics
    PRIVATE
        glm
        spdlog
        EnTT
        BulletDynamics
        BulletCollision
        BulletSoftBody
        LinearMath
        bullet3
)

target_compile_features(bsp-tri-physics
    PRIVATE
        cxx_std_20
        cxx_auto_type
        cxx_nullptr
        cxx_range_for
        cxx_thread_local
)

if(MINGW)
    target_link_options(bsp-tri-physics
        PUBLIC
            -static
    )
endif()

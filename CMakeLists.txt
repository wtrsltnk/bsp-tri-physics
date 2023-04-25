cmake_minimum_required(VERSION 3.7)

include(cmake/CPM.cmake)

project(genmap)

find_package(OPENGL REQUIRED)

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

add_executable(genmap
    camera.cpp
    camera.h
    entitycomponents.h
    genmapapp.cpp
    genmapapp.h
    hl1bspasset.cpp
    hl1bspasset.h
    hl1bsptypes.h
    hl1filesystem.cpp
    hl1filesystem.h
    hl1wadasset.cpp
    hl1wadasset.h
    hltexture.cpp
    hltexture.h
    hltypes.h
    include/application.h
    include/glad.c
    include/glad/glad.h
    include/glbuffer.h
    include/glshader.h
    include/stb_image.h
    include/stb_rect_pack.h
    main.cpp
    physicsservice.cpp
    physicsservice.hpp
    stb_image.cpp
    stb_rect_pack.cpp
    vertexarray.cpp
)

target_sources(genmap
    PRIVATE
        include/GL/wglext.h
)

target_include_directories(genmap
    PRIVATE
        include
        glad
)

target_link_libraries(genmap
    PRIVATE
        ${OPENGL_LIBRARIES}
        glm
        spdlog
        EnTT
        BulletDynamics
        BulletCollision
        BulletSoftBody
        LinearMath
        bullet3
)

target_compile_features(genmap
    PRIVATE
        cxx_std_17
        cxx_auto_type
        cxx_nullptr
        cxx_range_for
        cxx_thread_local
)

if(MINGW)
    target_link_options(genmap
        PUBLIC
            -static
    )
endif()

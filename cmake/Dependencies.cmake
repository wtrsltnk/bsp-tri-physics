
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

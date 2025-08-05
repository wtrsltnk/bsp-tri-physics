
CPMAddPackage(
    NAME glm
    GITHUB_REPOSITORY g-truc/glm
    GIT_TAG 1.0.1
)

CPMAddPackage(
    NAME EnTT
    VERSION 3.15.0
    GITHUB_REPOSITORY skypjack/entt
    # EnTT's CMakeLists screws with configuration options
    DOWNLOAD_ONLY True
)

if (EnTT_ADDED)
    add_library(EnTT INTERFACE)
    target_include_directories(EnTT INTERFACE ${EnTT_SOURCE_DIR}/src)
endif()

CPMAddPackage(
    NAME bullet
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

if (bullet_ADDED)
    add_library(bullet INTERFACE)
    target_include_directories(bullet INTERFACE ${bullet_SOURCE_DIR}/src)
endif()

CPMAddPackage(
    NAME imgui
    GIT_TAG v1.89.7
    GITHUB_REPOSITORY ocornut/imgui
    DOWNLOAD_ONLY True
)

if (imgui_ADDED)
    add_library(imgui)
    target_include_directories(
        imgui
        PUBLIC
            "${imgui_SOURCE_DIR}/"
            "${imgui_SOURCE_DIR}/examples/"
    )

    target_sources(
        imgui
        PUBLIC
            "${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp"
            "${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp"
            "${imgui_SOURCE_DIR}/imgui.cpp"
            "${imgui_SOURCE_DIR}/imgui_demo.cpp"
            "${imgui_SOURCE_DIR}/imgui_draw.cpp"
            "${imgui_SOURCE_DIR}/imgui_tables.cpp"
            "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
    )
endif()

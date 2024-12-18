#include "goldsrcviewerapp.h"

#include "assetmanager.h"
#include "physicsservice.hpp"
#include "renderers/opengl/openglrenderer.hpp"

#include <application.h>
#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_win32.h>
#include <ctime>
#include <filesystem>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/string_cast.hpp>
#include <imgui.h>
#include <spdlog/spdlog.h>
#include <stb_image.h>

bool showPhysicsDebug = false;

void EnableOpenGlDebug();

GoldSrcViewerApp::GoldSrcViewerApp()
{
    _fileSystem = std::make_unique<FileSystem>();
    _assets = std::make_unique<AssetManager>(_fileSystem.get());
}

void GoldSrcViewerApp::SetFilename(
    const char *root,
    const char *map)
{
    _fileSystem->FindRootFromFilePath(root);

    if (map == nullptr)
    {
        _fileSystem->FindRootFromFilePath(root);
        _map = root;
    }
    else
    {
        _map = map;
    }
}

bool GoldSrcViewerApp::Startup(
    void *windowHandle)
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_InitForOpenGL(windowHandle);
    ImGui_ImplOpenGL3_Init();

    spdlog::debug("Startup()");

    spdlog::info("{} @ {}", _fileSystem->Mod(), _fileSystem->Root().generic_string());

    _renderer = std::make_unique<OpenGlRenderer>();

    _physics = std::make_unique<PhysicsService>();

    _engine = std::make_unique<Engine>(_renderer.get(), _physics.get(), _assets.get());

    EnableOpenGlDebug();

    glClearColor(0.0f, 0.45f, 0.7f, 1.0f);

    _engine->Load(_map);

    return true;
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

struct ProcParams
{
    HWND hWnd;
    UINT uMsg;
    WPARAM wParam;
    LPARAM lParam;
};

bool GoldSrcViewerApp::RawEvent(
    void *eventPackage)
{
    auto procParams = reinterpret_cast<ProcParams *>(eventPackage);

    if (ImGui_ImplWin32_WndProcHandler(procParams->hWnd, procParams->uMsg, procParams->wParam, procParams->lParam))
    {
        return true;
    }

    return false;
}

bool show_demo_window = true;
bool show_another_window = false;
ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

bool GoldSrcViewerApp::Tick(
    std::chrono::microseconds time,
    const struct InputState &inputState)
{
    _engine->Update(time, inputState);

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    if (show_demo_window)
    {
        ImGui::ShowDemoWindow(&show_demo_window);
    }

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
    {
        static float f = 0.0f;
        static int counter = 0;

        ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!" and append into it.

        ImGui::Text("This is some useful text.");          // Display some text (you can use a format strings too)
        ImGui::Checkbox("Demo Window", &show_demo_window); // Edit bools storing our window open/close state
        ImGui::Checkbox("Another Window", &show_another_window);

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);             // Edit 1 float using a slider from 0.0f to 1.0f
        ImGui::ColorEdit3("clear color", (float *)&clear_color); // Edit 3 floats representing a color

        if (ImGui::Button("Button")) // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGuiIO &io = ImGui::GetIO();
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::End();
    }

    // 3. Show another simple window.
    if (show_another_window)
    {
        ImGui::Begin("Another Window", &show_another_window); // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
            show_another_window = false;
        ImGui::End();
    }

    // Rendering
    ImGui::Render();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    _engine->Render(time);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    return true; // to keep running
}

void GoldSrcViewerApp::Resize(
    int width,
    int height)
{
    glViewport(0, 0, width, height);

    _engine->SetProjectionMatrix(
        glm::perspective(glm::radians(70.0f), float(width) / float(height), 0.1f, 4096.0f));
}

void GoldSrcViewerApp::Destroy()
{
}

void OpenGLMessageCallback(
    unsigned source,
    unsigned type,
    unsigned id,
    unsigned severity,
    int length,
    const char *message,
    const void *userParam)
{
    (void)source;
    (void)type;
    (void)id;
    (void)length;
    (void)userParam;

    switch (severity)
    {
        case GL_DEBUG_SEVERITY_HIGH:
            spdlog::critical("{} - {}", source, message);
            return;
        case GL_DEBUG_SEVERITY_MEDIUM:
            spdlog::error("{} - {}", source, message);
            return;
        case GL_DEBUG_SEVERITY_LOW:
            spdlog::warn("{} - {}", source, message);
            return;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            spdlog::trace("{} - {}", source, message);
            return;
    }

    spdlog::debug("Unknown severity level!");
    spdlog::debug(message);
}

void EnableOpenGlDebug()
{
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(OpenGLMessageCallback, nullptr);

    glDebugMessageControl(
        GL_DONT_CARE,
        GL_DONT_CARE,
        GL_DEBUG_SEVERITY_NOTIFICATION,
        0,
        NULL,
        GL_FALSE);
}

//// pos x, y, z, color h, s, v
// float GenMapApp::vertices[216] = {
//     -0.5f,
//     -0.5f,
//     -0.5f,
//     0.4f,
//     0.6f,
//     1.0f,
//     0.5f,
//     -0.5f,
//     -0.5f,
//     0.4f,
//     0.6f,
//     1.0f,
//     0.5f,
//     0.5f,
//     -0.5f,
//     0.4f,
//     0.6f,
//     1.0f,
//     0.5f,
//     0.5f,
//     -0.5f,
//     0.4f,
//     0.6f,
//     1.0f,
//     -0.5f,
//     0.5f,
//     -0.5f,
//     0.4f,
//     0.6f,
//     1.0f,
//     -0.5f,
//     -0.5f,
//     -0.5f,
//     0.4f,
//     0.6f,
//     1.0f,

//    -0.5f,
//    -0.5f,
//    0.5f,
//    0.4f,
//    0.6f,
//    1.0f,
//    0.5f,
//    -0.5f,
//    0.5f,
//    0.4f,
//    0.6f,
//    1.0f,
//    0.5f,
//    0.5f,
//    0.5f,
//    0.4f,
//    0.6f,
//    1.0f,
//    0.5f,
//    0.5f,
//    0.5f,
//    0.4f,
//    0.6f,
//    1.0f,
//    -0.5f,
//    0.5f,
//    0.5f,
//    0.4f,
//    0.6f,
//    1.0f,
//    -0.5f,
//    -0.5f,
//    0.5f,
//    0.4f,
//    0.6f,
//    1.0f,

//    -0.5f,
//    0.5f,
//    0.5f,
//    0.5f,
//    0.6f,
//    1.0f,
//    -0.5f,
//    0.5f,
//    -0.5f,
//    0.5f,
//    0.6f,
//    1.0f,
//    -0.5f,
//    -0.5f,
//    -0.5f,
//    0.5f,
//    0.6f,
//    1.0f,
//    -0.5f,
//    -0.5f,
//    -0.5f,
//    0.5f,
//    0.6f,
//    1.0f,
//    -0.5f,
//    -0.5f,
//    0.5f,
//    0.5f,
//    0.6f,
//    1.0f,
//    -0.5f,
//    0.5f,
//    0.5f,
//    0.5f,
//    0.6f,
//    1.0f,

//    0.5f,
//    0.5f,
//    0.5f,
//    0.5f,
//    0.6f,
//    1.0f,
//    0.5f,
//    0.5f,
//    -0.5f,
//    0.5f,
//    0.6f,
//    1.0f,
//    0.5f,
//    -0.5f,
//    -0.5f,
//    0.5f,
//    0.6f,
//    1.0f,
//    0.5f,
//    -0.5f,
//    -0.5f,
//    0.5f,
//    0.6f,
//    1.0f,
//    0.5f,
//    -0.5f,
//    0.5f,
//    0.5f,
//    0.6f,
//    1.0f,
//    0.5f,
//    0.5f,
//    0.5f,
//    0.5f,
//    0.6f,
//    1.0f,

//    -0.5f,
//    -0.5f,
//    -0.5f,
//    0.6f,
//    0.6f,
//    1.0f,
//    0.5f,
//    -0.5f,
//    -0.5f,
//    0.6f,
//    0.6f,
//    1.0f,
//    0.5f,
//    -0.5f,
//    0.5f,
//    0.6f,
//    0.6f,
//    1.0f,
//    0.5f,
//    -0.5f,
//    0.5f,
//    0.6f,
//    0.6f,
//    1.0f,
//    -0.5f,
//    -0.5f,
//    0.5f,
//    0.6f,
//    0.6f,
//    1.0f,
//    -0.5f,
//    -0.5f,
//    -0.5f,
//    0.6f,
//    0.6f,
//    1.0f,

//    -0.5f,
//    0.5f,
//    -0.5f,
//    0.6f,
//    0.6f,
//    1.0f,
//    0.5f,
//    0.5f,
//    -0.5f,
//    0.6f,
//    0.6f,
//    1.0f,
//    0.5f,
//    0.5f,
//    0.5f,
//    0.6f,
//    0.6f,
//    1.0f,
//    0.5f,
//    0.5f,
//    0.5f,
//    0.6f,
//    0.6f,
//    1.0f,
//    -0.5f,
//    0.5f,
//    0.5f,
//    0.6f,
//    0.6f,
//    1.0f,
//    -0.5f,
//    0.5f,
//    -0.5f,
//    0.6f,
//    0.6f,
//    1.0f,
//};

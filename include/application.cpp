#include "application.h"

bool IsKeyboardButtonPushed(
    const struct InputState &inputState,
    KeyboardButtons button)
{
    return inputState.KeyboardButtonStates[button] && inputState.KeyboardButtonStates[button] != inputState.PreviousState->KeyboardButtonStates[button];
}

bool IsMouseButtonPushed(
    const struct InputState &inputState,
    MouseButtons button)
{

    return inputState.MouseButtonStates[button] && inputState.MouseButtonStates[button] != inputState.PreviousState->MouseButtonStates[button];
}

#include <glad/glad.h>

#ifdef _WIN32

#include <GL/wglext.h>
#include <windowsx.h>
#include <sstream>

#define EXAMPLE_NAME "genmap"

class Win32Application
{
public:
    bool Startup(
        std::function<bool()> intialize,
        std::function<void(int width, int height)> resize,
        std::function<void()> destroy);

    int Run(
        std::function<bool(std::chrono::microseconds time, const struct InputState &inputState)> tick);

private:
    std::function<void(int width, int height)> _resize;
    std::function<void()> _destroy;
    HINSTANCE _hInstance;
    HWND _hWnd;
    HDC _hDC;
    HGLRC _hRC;
    PFNWGLCREATECONTEXTATTRIBSARBPROC _pfnGlxCreateContext;
    InputState _inputState;
    InputState _previousInputState;

    virtual void Destroy(
        const char *errorMessage = nullptr);

    LRESULT CALLBACK objectProc(
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam);

    static LRESULT CALLBACK staticProc(
        HWND hWnd,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam);
};

bool Win32Application::Startup(
    std::function<bool()> intialize,
    std::function<void(int width, int height)> resize,
    std::function<void()> destroy)
{
    _resize = resize;
    _destroy = destroy;
    _hInstance = GetModuleHandle(nullptr);

    WNDCLASS wc;

    if (GetClassInfo(_hInstance, EXAMPLE_NAME, &wc) == FALSE)
    {
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        wc.lpfnWndProc = (WNDPROC)Win32Application::staticProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = _hInstance;
        wc.hIcon = LoadIcon(nullptr, IDI_WINLOGO);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = nullptr;
        wc.lpszMenuName = nullptr;
        wc.lpszClassName = EXAMPLE_NAME;

        if (RegisterClass(&wc) == FALSE)
        {
            Destroy("Failed to register window class");
            return false;
        }
    }

    _hWnd = CreateWindowEx(
        WS_EX_APPWINDOW,
        EXAMPLE_NAME, EXAMPLE_NAME,
        WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT,
        nullptr, nullptr,
        _hInstance, (VOID *)this);

    if (_hWnd == 0)
    {
        Destroy("Failed to create window");
        return false;
    }

    _hDC = GetDC(_hWnd);

    if (_hDC == 0)
    {
        Destroy("Failed to get device context");
        return false;
    }

    static PIXELFORMATDESCRIPTOR pfd =
        {
            sizeof(PIXELFORMATDESCRIPTOR), // Size Of This Pixel Format Descriptor
            1,                             // Version Number
            PFD_DRAW_TO_WINDOW |           // Format Must Support Window
                PFD_SUPPORT_OPENGL |       // Format Must Support CodeGL
                PFD_DOUBLEBUFFER,          // Must Support Double Buffering
            PFD_TYPE_RGBA,                 // Request An RGBA Format
            32,                            // Select Our Color Depth
            0, 0, 0, 0, 0, 0,              // Color Bits Ignored
            0,                             // No Alpha Buffer
            0,                             // Shift Bit Ignored
            0,                             // No Accumulation Buffer
            0, 0, 0, 0,                    // Accumulation Bits Ignored
            24,                            // 24Bit Z-Buffer (Depth Buffer)
            0,                             // No Stencil Buffer
            0,                             // No Auxiliary Buffer
            PFD_MAIN_PLANE,                // Main Drawing Layer
            0,                             // Reserved
            0, 0, 0                        // Layer Masks Ignored
        };

    auto pixelFormat = ChoosePixelFormat(_hDC, &pfd);
    if (pixelFormat == false)
    {
        Destroy("Failed to choose pixel format");
        return false;
    }

    if (SetPixelFormat(_hDC, pixelFormat, &pfd) == FALSE)
    {
        Destroy("Failed to set pixel format");
        return false;
    }

    _hRC = wglCreateContext(_hDC);

    if (_hRC == 0)
    {
        Destroy("Failed to create clasic opengl context (v1.0)");
        return false;
    }

    if (!wglMakeCurrent(_hDC, _hRC))
    {
        wglDeleteContext(_hRC);
        return false;
    }

    _pfnGlxCreateContext = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
    if (_pfnGlxCreateContext != nullptr)
    {
        GLint attribList[] =
            {
                WGL_CONTEXT_MAJOR_VERSION_ARB,
                4,
                WGL_CONTEXT_MINOR_VERSION_ARB,
                6,
                WGL_CONTEXT_FLAGS_ARB,
                0,
                0,
            };

        _hRC = _pfnGlxCreateContext(_hDC, 0, attribList);
        if (_hRC == 0)
        {
            Destroy("Failed to create modern opengl context (v4.6)");

            return false;
        }
    }

    wglMakeCurrent(_hDC, _hRC);

    gladLoadGL();

    spdlog::debug("GL_VERSION                  : {0}", glGetString(GL_VERSION));
    spdlog::debug("GL_SHADING_LANGUAGE_VERSION : {0}", glGetString(GL_SHADING_LANGUAGE_VERSION));
    spdlog::debug("GL_RENDERER                 : {0}", glGetString(GL_RENDERER));
    spdlog::debug("GL_VENDOR                   : {0}", glGetString(GL_VENDOR));

    if (!intialize())
    {
        Destroy("Initialize failed");
        return false;
    }

    ShowWindow(_hWnd, SW_SHOW);
    SetForegroundWindow(_hWnd);
    SetFocus(_hWnd);

    return true;
}

std::chrono::microseconds GetDiffTime()
{
    static std::chrono::time_point<std::chrono::steady_clock> prev = std::chrono::steady_clock::now();

    auto now = std::chrono::steady_clock::now();

    auto result = std::chrono::duration_cast<std::chrono::microseconds>(now - prev);

    prev = now;

    return result;
}

std::chrono::microseconds CurrentTime()
{
    auto now = std::chrono::system_clock::now().time_since_epoch();

    return std::chrono::duration_cast<std::chrono::microseconds>(now);
}

int Win32Application::Run(
    std::function<bool(std::chrono::microseconds time, const struct InputState &inputState)> tick)
{
    bool running = true;
    int fps = 0;
    long long fpsTime = 0;

    _inputState.PreviousState = &_previousInputState;

    while (running)
    {
        memcpy(&_previousInputState, &_inputState, sizeof(InputState));

        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                break;
            }
            else
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        if (IsWindowVisible(_hWnd) == FALSE)
        {
            break;
        }

        POINT point;
        GetCursorPos(&point);

        RECT rect;
        GetWindowRect(this->_hWnd, &rect);

        auto centerX = rect.left + ((rect.right - rect.left) / 2);
        auto centerY = rect.top + ((rect.bottom - rect.top) / 2);

        SetCursorPos(centerX, centerY);

        auto time = GetDiffTime();

        fpsTime += time.count();
        fps++;
        if (fpsTime > 1000)
        {
            std::stringstream ss;
            ss << "FPS: " <<  fps;
            SetWindowText(_hWnd, ss.str().c_str());

            fps = 0;
            fpsTime -= 1000;
        }

        running = tick(time, _inputState);

        _inputState.MousePointerPosition[0] = centerX - point.x;
        _inputState.MousePointerPosition[1] = centerY - point.y;

        SwapBuffers(_hDC);
    }

    _destroy();

    return 0;
}

void Win32Application::Destroy(
    const char *errorMessage)
{
    if (errorMessage != nullptr)
    {
        spdlog::error(errorMessage);
    }

    _destroy();

    UnregisterClass(EXAMPLE_NAME, _hInstance);

    if (_hDC != 0)
    {
        ReleaseDC(_hWnd, _hDC);
        _hDC = 0;
    }

    if (_hWnd != 0)
    {
        DestroyWindow(_hWnd);
        _hWnd = 0;
    }
}

LRESULT CALLBACK Win32Application::objectProc(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_SIZE:
        {
            auto width = LOWORD(lParam);
            auto height = HIWORD(lParam);

            this->_resize(width, height);

            RECT rect;
            GetWindowRect(this->_hWnd, &rect);
            SetCursorPos(
                rect.left + ((rect.right - rect.left) / 2),
                rect.top + ((rect.bottom - rect.top) / 2));
            ClipCursor(&rect);
            ShowCursor(FALSE);

            break;
        }
        case WM_DESTROY:
        {
            ClipCursor(NULL);
            ShowCursor(TRUE);
            break;
        }
        case WM_MOUSEMOVE:
        {
            static int lastMouseX = GET_X_LPARAM(lParam);
            static int lastMouseY = GET_Y_LPARAM(lParam);

            _inputState.MousePointerPosition[0] += lastMouseX - GET_X_LPARAM(lParam);
            _inputState.MousePointerPosition[1] += lastMouseY - GET_Y_LPARAM(lParam);

            lastMouseX = GET_X_LPARAM(lParam);
            lastMouseY = GET_Y_LPARAM(lParam);

            break;
        }
        case WM_LBUTTONDOWN:
        {
            _inputState.MouseButtonStates[MouseButtons::LeftButton] = true;
            break;
        }
        case WM_LBUTTONUP:
        {
            _inputState.MouseButtonStates[MouseButtons::LeftButton] = false;
            break;
        }
        case WM_RBUTTONDOWN:
        {
            _inputState.MouseButtonStates[MouseButtons::RightButton] = true;
            break;
        }
        case WM_RBUTTONUP:
        {
            _inputState.MouseButtonStates[MouseButtons::RightButton] = false;
            break;
        }
        case WM_MBUTTONDOWN:
        {
            _inputState.MouseButtonStates[MouseButtons::MiddleButton] = true;
            break;
        }
        case WM_MBUTTONUP:
        {
            _inputState.MouseButtonStates[MouseButtons::MiddleButton] = false;
            break;
        }
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            auto scancode = (HIWORD(lParam) & (KF_EXTENDED | 0xff));
            if (!scancode)
            {
                // NOTE: Some synthetic key messages have a scancode of zero
                // HACK: Map the virtual key back to a usable scancode
                scancode = MapVirtualKeyW((UINT)wParam, MAPVK_VK_TO_VSC);
            }

            switch (scancode)
            {
                case 0x00B:
                    _inputState.KeyboardButtonStates[Key0] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x002:
                    _inputState.KeyboardButtonStates[Key1] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x003:
                    _inputState.KeyboardButtonStates[Key2] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x004:
                    _inputState.KeyboardButtonStates[Key3] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x005:
                    _inputState.KeyboardButtonStates[Key4] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x006:
                    _inputState.KeyboardButtonStates[Key5] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x007:
                    _inputState.KeyboardButtonStates[Key6] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x008:
                    _inputState.KeyboardButtonStates[Key7] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x009:
                    _inputState.KeyboardButtonStates[Key8] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x00A:
                    _inputState.KeyboardButtonStates[Key9] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x01E:
                    _inputState.KeyboardButtonStates[KeyA] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x030:
                    _inputState.KeyboardButtonStates[KeyB] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x02E:
                    _inputState.KeyboardButtonStates[KeyC] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x020:
                    _inputState.KeyboardButtonStates[KeyD] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x012:
                    _inputState.KeyboardButtonStates[KeyE] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x021:
                    _inputState.KeyboardButtonStates[KeyF] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x022:
                    _inputState.KeyboardButtonStates[KeyG] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x023:
                    _inputState.KeyboardButtonStates[KeyH] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x017:
                    _inputState.KeyboardButtonStates[KeyI] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x024:
                    _inputState.KeyboardButtonStates[KeyJ] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x025:
                    _inputState.KeyboardButtonStates[KeyK] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x026:
                    _inputState.KeyboardButtonStates[KeyL] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x032:
                    _inputState.KeyboardButtonStates[KeyM] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x031:
                    _inputState.KeyboardButtonStates[KeyN] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x018:
                    _inputState.KeyboardButtonStates[KeyO] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x019:
                    _inputState.KeyboardButtonStates[KeyP] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x010:
                    _inputState.KeyboardButtonStates[KeyQ] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x013:
                    _inputState.KeyboardButtonStates[KeyR] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x01F:
                    _inputState.KeyboardButtonStates[KeyS] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x014:
                    _inputState.KeyboardButtonStates[KeyT] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x016:
                    _inputState.KeyboardButtonStates[KeyU] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x02F:
                    _inputState.KeyboardButtonStates[KeyV] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x011:
                    _inputState.KeyboardButtonStates[KeyW] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x02D:
                    _inputState.KeyboardButtonStates[KeyX] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x015:
                    _inputState.KeyboardButtonStates[KeyY] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x02C:
                    _inputState.KeyboardButtonStates[KeyZ] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;

                case 0x028:
                    _inputState.KeyboardButtonStates[KeyApostrophe] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x02B:
                    _inputState.KeyboardButtonStates[KeyBackslash] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x033:
                    _inputState.KeyboardButtonStates[KeyComma] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x00D:
                    _inputState.KeyboardButtonStates[KeyEqual] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x029:
                    _inputState.KeyboardButtonStates[KeyGraveAccent] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x01A:
                    _inputState.KeyboardButtonStates[KeyLeftBracket] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x00C:
                    _inputState.KeyboardButtonStates[KeyMinus] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x034:
                    _inputState.KeyboardButtonStates[KeyPeriod] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x01B:
                    _inputState.KeyboardButtonStates[KeyRightBracket] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x027:
                    _inputState.KeyboardButtonStates[KeySemicolon] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x035:
                    _inputState.KeyboardButtonStates[KeySlash] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x056:
                    _inputState.KeyboardButtonStates[KeyWorld2] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;

                case 0x00E:
                    _inputState.KeyboardButtonStates[KeyBackspace] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x153:
                    _inputState.KeyboardButtonStates[KeyDelete] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x14F:
                    _inputState.KeyboardButtonStates[KeyEnd] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x01C:
                    _inputState.KeyboardButtonStates[KeyEnter] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x001:
                    _inputState.KeyboardButtonStates[KeyEscape] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x147:
                    _inputState.KeyboardButtonStates[KeyHome] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x152:
                    _inputState.KeyboardButtonStates[KeyInsert] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x15D:
                    _inputState.KeyboardButtonStates[KeyMenu] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x151:
                    _inputState.KeyboardButtonStates[KeyPageDown] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x149:
                    _inputState.KeyboardButtonStates[KeyPageUp] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x045:
                    _inputState.KeyboardButtonStates[KeyPause] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x146:
                    _inputState.KeyboardButtonStates[KeyPause] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x039:
                    _inputState.KeyboardButtonStates[KeySpace] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x00F:
                    _inputState.KeyboardButtonStates[KeyTab] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x03A:
                    _inputState.KeyboardButtonStates[KeyCapsLock] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x145:
                    _inputState.KeyboardButtonStates[KeyNumLock] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x046:
                    _inputState.KeyboardButtonStates[KeyScrollLock] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x03B:
                    _inputState.KeyboardButtonStates[KeyF1] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x03C:
                    _inputState.KeyboardButtonStates[KeyF2] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x03D:
                    _inputState.KeyboardButtonStates[KeyF3] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x03E:
                    _inputState.KeyboardButtonStates[KeyF4] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x03F:
                    _inputState.KeyboardButtonStates[KeyF5] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x040:
                    _inputState.KeyboardButtonStates[KeyF6] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x041:
                    _inputState.KeyboardButtonStates[KeyF7] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x042:
                    _inputState.KeyboardButtonStates[KeyF8] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x043:
                    _inputState.KeyboardButtonStates[KeyF9] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x044:
                    _inputState.KeyboardButtonStates[KeyF10] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x057:
                    _inputState.KeyboardButtonStates[KeyF11] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x058:
                    _inputState.KeyboardButtonStates[KeyF12] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x064:
                    _inputState.KeyboardButtonStates[KeyF13] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x065:
                    _inputState.KeyboardButtonStates[KeyF14] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x066:
                    _inputState.KeyboardButtonStates[KeyF15] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x067:
                    _inputState.KeyboardButtonStates[KeyF16] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x068:
                    _inputState.KeyboardButtonStates[KeyF17] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x069:
                    _inputState.KeyboardButtonStates[KeyF18] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x06A:
                    _inputState.KeyboardButtonStates[KeyF19] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x06B:
                    _inputState.KeyboardButtonStates[KeyF20] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x06C:
                    _inputState.KeyboardButtonStates[KeyF21] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x06D:
                    _inputState.KeyboardButtonStates[KeyF22] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x06E:
                    _inputState.KeyboardButtonStates[KeyF23] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x076:
                    _inputState.KeyboardButtonStates[KeyF24] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x038:
                    _inputState.KeyboardButtonStates[KeyLeftAlt] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x01D:
                    _inputState.KeyboardButtonStates[KeyLeftControl] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x02A:
                    _inputState.KeyboardButtonStates[KeyLeftShift] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x15B:
                    _inputState.KeyboardButtonStates[KeyLeftSuper] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x137:
                    _inputState.KeyboardButtonStates[KeyPrintScreen] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x138:
                    _inputState.KeyboardButtonStates[KeyRightAlt] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x11D:
                    _inputState.KeyboardButtonStates[KeyRightControl] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x036:
                    _inputState.KeyboardButtonStates[KeyRightShift] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x15C:
                    _inputState.KeyboardButtonStates[KeyRightSuper] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x150:
                    _inputState.KeyboardButtonStates[KeyDown] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x14B:
                    _inputState.KeyboardButtonStates[KeyLeft] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x14D:
                    _inputState.KeyboardButtonStates[KeyRight] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x148:
                    _inputState.KeyboardButtonStates[KeyUp] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;

                case 0x052:
                    _inputState.KeyboardButtonStates[KeyKp0] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x04F:
                    _inputState.KeyboardButtonStates[KeyKp1] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x050:
                    _inputState.KeyboardButtonStates[KeyKp2] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x051:
                    _inputState.KeyboardButtonStates[KeyKp3] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x04B:
                    _inputState.KeyboardButtonStates[KeyKp4] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x04C:
                    _inputState.KeyboardButtonStates[KeyKp5] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x04D:
                    _inputState.KeyboardButtonStates[KeyKp6] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x047:
                    _inputState.KeyboardButtonStates[KeyKp7] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x048:
                    _inputState.KeyboardButtonStates[KeyKp8] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x049:
                    _inputState.KeyboardButtonStates[KeyKp9] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x04E:
                    _inputState.KeyboardButtonStates[KeyKpAdd] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x053:
                    _inputState.KeyboardButtonStates[KeyKpDecimal] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x135:
                    _inputState.KeyboardButtonStates[KeyKpDivide] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x11C:
                    _inputState.KeyboardButtonStates[KeyKpEnter] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x059:
                    _inputState.KeyboardButtonStates[KeyKpEqual] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x037:
                    _inputState.KeyboardButtonStates[KeyKpMultiply] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x04A:
                    _inputState.KeyboardButtonStates[KeyKpSubtract] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
            }
            break;
        }
    }
    return DefWindowProc(this->_hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK Win32Application::staticProc(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    Win32Application *app = nullptr;

    if (uMsg == WM_NCCREATE)
    {
        app = reinterpret_cast<Win32Application *>(((LPCREATESTRUCT)lParam)->lpCreateParams);

        if (app != nullptr)
        {
            app->_hWnd = hWnd;

            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<long long>(app));

            return app->objectProc(uMsg, wParam, lParam);
        }
    }
    else
    {
        app = reinterpret_cast<Win32Application *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

        if (app != nullptr)
        {
            return app->objectProc(uMsg, wParam, lParam);
        }
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

Win32Application *CreateApplication(
    std::function<bool()> initialize,
    std::function<void(int width, int height)> resize,
    std::function<void()> destroy)
{
    static Win32Application app;

    if (app.Startup(initialize, resize, destroy))
    {
        return &app;
    }

    spdlog::error("Create application failed");

    exit(0);

    return nullptr;
}
#endif // _WIN32

#ifdef __linux__
#endif // __linux__

void InputState::OnKeyboardButtonDown(
    KeyboardButtons button,
    std::function<void()> func)
{
    if (PreviousState == nullptr)
    {
        return;
    }

    if (KeyboardButtonStates[button] && KeyboardButtonStates[button] != PreviousState->KeyboardButtonStates[button])
    {
        func();
    }
}

void InputState::OnKeyboardButtonUp(
    KeyboardButtons button,
    std::function<void()> func)
{
    if (PreviousState == nullptr)
    {
        return;
    }

    if (!KeyboardButtonStates[button] && KeyboardButtonStates[button] != PreviousState->KeyboardButtonStates[button])
    {
        func();
    }
}

void InputState::OnMouseButtonDown(
    MouseButtons button,
    std::function<void()> func)
{
    if (PreviousState == nullptr)
    {
        return;
    }

    if (MouseButtonStates[button] && MouseButtonStates[button] != PreviousState->MouseButtonStates[button])
    {
        func();
    }
}

void InputState::OnMouseButtonUp(
    MouseButtons button,
    std::function<void()> func)
{
    if (PreviousState == nullptr)
    {
        return;
    }

    if (!MouseButtonStates[button] && MouseButtonStates[button] != PreviousState->MouseButtonStates[button])
    {
        func();
    }
}

void InputState::SetMousePointerPosition(
    int x,
    int y)
{
}

int Run(IApplication *t)
{
    auto app = CreateApplication(
        [&]() { return t->Startup(); },
        [&](int w, int h) { t->Resize(w, h); },
        [&]() { t->Destroy(); });

    return app->Run([&](std::chrono::microseconds time, const struct InputState &inputState) { return t->Tick(time, inputState); });
}

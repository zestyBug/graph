#include <iostream>
#include <thread>
#include <mutex>
#include <windows.h>

#include "Core.hpp"
#include "glcorearb.h"
#include "imgui/imgui.h"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui/imgui_internal.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_my.hpp"
#include "Timer.hpp"

typedef HGLRC(WINAPI* PFNWGLCREATECONTEXTATTRIBSARBPROC) (HDC hDC, HGLRC hShareContext, const int* attribList);
typedef BOOL(WINAPI* PFNWGLCHOOSEPIXELFORMATARBPROC) (HDC hdc, const int* piAttribIList, const FLOAT* pfAttribFList, UINT nMaxFormats, int* piFormats, UINT* nNumFormats);
typedef const char* (WINAPI* PFNWGLGETEXTENSIONSSTRINGARBPROC) (HDC hdc);
//get wgl*ARB function
PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;
PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB;
PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB;

#define WGL_DRAW_TO_WINDOW_ARB            0x2001
#define WGL_SUPPORT_OPENGL_ARB            0x2010
#define WGL_DOUBLE_BUFFER_ARB             0x2011
#define WGL_PIXEL_TYPE_ARB                0x2013
#define WGL_COLOR_BITS_ARB                0x2014
#define WGL_ALPHA_BITS_ARB                0x201B
#define WGL_DEPTH_BITS_ARB                0x2022
#define WGL_STENCIL_BITS_ARB              0x2023
#define WGL_TYPE_RGBA_ARB                 0x202B
#define WGL_CONTEXT_DEBUG_BIT_ARB         0x00000001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB 0x00000002
#define WGL_CONTEXT_MAJOR_VERSION_ARB     0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB     0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB       0x2093
#define WGL_CONTEXT_FLAGS_ARB             0x2094
#define ERROR_INVALID_VERSION_ARB         0x2095
#define WGL_CONTEXT_PROFILE_MASK_ARB      0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB  0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002

void dummy_windows()
{
    HWND hWnd;
    HDC hDC;
    HGLRC hGLRC;

    hWnd = ::CreateWindow("window", "Fake Window",
        WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        //WS_CAPTION | WS_POPUPWINDOW | WS_VISIBLE,
        0, 0, 10, 10, NULL, NULL, NULL, NULL);
    hDC = GetDC(hWnd);
    if (!hDC)
        throw std::exception("unable to get window device content (dummy)\n");
    // Create dummy GL context that will be used to create the real context
    // We will use the dummy context to use wgl functions to create a ARB supported context
    PIXELFORMATDESCRIPTOR pfd;
    int iFormat;
    /* set the pixel format for the DC */
    ZeroMemory(&pfd, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cDepthBits = 16;
    pfd.cAlphaBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;
    iFormat = ChoosePixelFormat(hDC, &pfd);
    if (!SetPixelFormat(hDC, iFormat, &pfd)) {
        printf("unable to set window pixel format (dummy) %lu\n"); exit(1);
    }
    hGLRC = wglCreateContext(hDC);
    if (!hGLRC) {
        printf("unable to create opengl context (dummy)\n"); exit(1);
    }
    if (!wglMakeCurrent(hDC, hGLRC)) {
        printf("unable to make window current (dummy)\n"); exit(1);
    }
    //get wgl*ARB function
    wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
    wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
    wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringARB");
    //glGetString = (PFNGLGETSTRING) wglGetProcAddress("glGetString");

    if (wglCreateContextAttribsARB == NULL
        || wglChoosePixelFormatARB == NULL
        || wglGetExtensionsStringARB == NULL
        //|| glGetString == NULL
        ) {
        printf("failed to load wgl*ARB functions"); exit(1);
    }

    if (glwInitialize(0x303)) {
        printf("glwInitialize failed\n"); exit(1);
    }

    //printf("%s\n",wglGetExtensionsStringARB(hDC) );//wglGetCurrentDC()
    //printf("%s\n",glGetString(GL_EXTENSIONS) );//wglGetCurrentDC()
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hGLRC);
    ReleaseDC(hWnd, hDC);
    DestroyWindow(hWnd);
}



LRESULT CALLBACK WndProc
(HWND _hWnd, UINT message, WPARAM wParam, LPARAM lParam);


Core        rc{};
Timer       DeltaTime;
HWND		hWnd;
HDC			hDC;
bool		isRunning = 1;
bool		Rendered = 1;
std::mutex render_mutex;
// in ms
double		SleepLenght = 1000.0 / 30.0;
// previous time screen was rendered
double prevouis_time = 0;
const char* WINDOW_CLASS_NAME = "Graph_Project_Window";
const int window_initial_width = 700, window_initial_height = 500;
int width = window_initial_width, height = window_initial_height;
uint32_t MouseButtonsDown = 0;
bool MouseTracked = false;

#ifdef DEBUG
HINSTANCE hInstance = 0;
int main()
#else
int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    const int iCmdShow)
#endif
    {
    rc.OnInit();

    HGLRC		hGLRC;
    HWND		console;

    WNDCLASS	wc;
    //wc.cbSize = sizeof(wc);
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = WINDOW_CLASS_NAME;
    wc.lpszMenuName = NULL;
    //wc.hIconSm = NULL;
    RegisterClass(&wc);

    dummy_windows();

    console = GetConsoleWindow();
    /* create main window */
    hWnd = CreateWindowEx(
        WS_EX_CLIENTEDGE, WINDOW_CLASS_NAME, "OpenGL Sample",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX |
        WS_CAPTION | WS_POPUPWINDOW | WS_VISIBLE | WS_SIZEBOX | WS_SYSMENU  
        ,
        100, 100, window_initial_width, window_initial_height,
        NULL, NULL, hInstance, NULL);
    if (!hWnd) {
        printf("unable to create window"); exit(1);
    }
    ShowWindow(hWnd, true);
    //ShowWindow(console, true);
    hDC = GetDC(hWnd);
    if (!hDC) {
        printf("unable to get window device context"); exit(1);
    }

    const int pixel_format_attribs[] =
    {
        WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
        WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
        WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
        WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
        WGL_COLOR_BITS_ARB, 24,
        WGL_DEPTH_BITS_ARB, 16,
        WGL_ALPHA_BITS_ARB, 8,
        //WGL_STENCIL_BITS_ARB, 8,
        0, // End
    };
    int pixel_format;
    UINT num_formats;
    if (!wglChoosePixelFormatARB(hDC, pixel_format_attribs, NULL, 1, &pixel_format, &num_formats) || !num_formats) {
        printf("Failed to choose pixel format. \n(may your computer does not support 24bit colors + 8bit alpha and 16 bit depth)\n"); exit(1);
    }

    PIXELFORMATDESCRIPTOR pfd2;
    if (!DescribePixelFormat(hDC, pixel_format, sizeof(pfd2), &pfd2)) {
        printf("Failed to describe pixel format.\n"); exit(1);
    }
    //the unfortinate is you can only call SetPixelFormat one per HWND, so you must create two window
    if (!SetPixelFormat(hDC, pixel_format, &pfd2)) {
        printf("Failed to set pixel format.\n"); exit(1);
    }


    /* create and enable the ARB OpenGL render context (RC) */
    int contextFlagsGL = 0;
    contextFlagsGL = WGL_CONTEXT_DEBUG_BIT_ARB;
    int gl_context_attribs[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
        WGL_CONTEXT_MINOR_VERSION_ARB, 3,
        WGL_CONTEXT_FLAGS_ARB, contextFlagsGL,
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        0
    };
    hGLRC = wglCreateContextAttribsARB(hDC, NULL, gl_context_attribs);
    if (!hGLRC) {
        printf("unable to create openglv3.1 context %lu\n", GetLastError()); exit(1);
    }

    if (!wglMakeCurrent(hDC, hGLRC)) {
        printf("unable to make current context %lu\n", GetLastError()); exit(1);
    }

    rc.OnCreate();

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable some options
    IM_ASSERT(io.BackendPlatformUserData == nullptr && "Already initialized a platform backend!");
    io.BackendPlatformUserData = (void*)IM_NEW(ImGui_ImplMy_Data)();
    io.BackendPlatformName = "imgui_impl_my";
    io.DisplaySize = ImVec2(width, height);

    ImGui::GetMainViewport()->PlatformHandleRaw = (void*)hWnd;

    ImGui_ImplOpenGL3_Init();

    ImGui::StyleColorsLight();

    DeltaTime.init();

    {
        RECT rect = { 0, 0, 0, 0 };
        ::GetClientRect(hWnd, &rect);
        width = rect.right - rect.left;
        height = rect.bottom - rect.top;
        io.DisplaySize = ImVec2((float)width, (float)height);
    }

    MSG msg;
    glClearColor(1.0, 0, 0, 0.5);
    glClearDepth(0.0);
    rc.OnStart();
    DeltaTime.start_point();
    prevouis_time = DeltaTime.now();


    while (isRunning)
    {
        /* check for messages in the queue */
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            // WM_KEYDOWN and WM_KEYUP ==> WM_CHAR, wont replace but add new message to the queue
            TranslateMessage(&msg);
            // pass the message to the WndProc
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                isRunning = 0;
        }
        // dont render until no event remaind
        else {
            //RedrawWindow(hWnd, NULL, NULL, RDW_INTERNALPAINT);
        }
    }
    rc.OnStop();
    ImGui_ImplOpenGL3_Shutdown();
    rc.OnDestroy();
    ImGui::DestroyContext();
    glwDestroy();

    wglDeleteContext(hGLRC);
    ReleaseDC(hWnd, hDC);
    ::DestroyWindow(hWnd);
    //printf("\n%s\n",terminal.c_str());
    return 0;
}













#define IM_VK_KEYPAD_ENTER      (VK_RETURN + 256)
ImGuiKey ImGui_ImplWin32_VirtualKeyToImGuiKey(WPARAM wParam);

LRESULT CALLBACK WndProc
(HWND _hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    //printEvent(message);
    if (ImGui::GetCurrentContext())
    {
        ImGuiIO& io = ImGui::GetIO();
        switch (message)
        {
        case WM_SIZE://WM_SIZING
        {
            RECT rect = { 0, 0, 0, 0 };
            ::GetClientRect(hWnd, &rect);
            width = rect.right - rect.left;
            height = rect.bottom - rect.top;
            io.DisplaySize = ImVec2((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));
            break;
        }
        case WM_MOUSEMOVE:
            // We need to call TrackMouseEvent in order to receive WM_MOUSELEAVE events
            if (!MouseTracked)
            {
                TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hWnd, 0 };
                ::TrackMouseEvent(&tme);
                MouseTracked = true;
            }
            io.AddMousePosEvent((float)LOWORD(lParam), (float)HIWORD(lParam));
            break;
        case WM_MOUSELEAVE:
            MouseTracked = false;
            io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
            break;
        case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
        case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK:
        {
            int button = 0;
            if (message == WM_LBUTTONDOWN || message == WM_LBUTTONDBLCLK) { button = 0; }
            else if (message == WM_RBUTTONDOWN || message == WM_RBUTTONDBLCLK) { button = 1; }
            else if (message == WM_MBUTTONDOWN || message == WM_MBUTTONDBLCLK) { button = 2; }
            else if (message == WM_XBUTTONDOWN || message == WM_XBUTTONDBLCLK) { button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? 3 : 4; }
            if (MouseButtonsDown == 0 && ::GetCapture() == nullptr)
                ::SetCapture(hWnd);
            MouseButtonsDown |= 1 << button;
            io.AddMouseButtonEvent(button, true);
            break;
        }
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        case WM_XBUTTONUP:
        {
            int button = 0;
            if (message == WM_LBUTTONUP) { button = 0; }
            else if (message == WM_RBUTTONUP) { button = 1; }
            else if (message == WM_MBUTTONUP) { button = 2; }
            else if (message == WM_XBUTTONUP) { button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? 3 : 4; }
            MouseButtonsDown &= ~(1 << button);
            if (MouseButtonsDown == 0 && ::GetCapture() == hWnd)
                ::ReleaseCapture();
            io.AddMouseButtonEvent(button, false);
            break;
        }
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        {
            if (wParam < 256)
            {
                io.AddKeyEvent(ImGuiMod_Ctrl, GetKeyState(VK_CONTROL) & 0x8000);
                io.AddKeyEvent(ImGuiMod_Shift, GetKeyState(VK_SHIFT) & 0x8000);
                io.AddKeyEvent(ImGuiMod_Alt, GetKeyState(VK_MENU) & 0x8000);
                io.AddKeyEvent(ImGuiMod_Super, GetKeyState(VK_APPS) & 0x8000);

                if ((wParam == VK_RETURN) && (HIWORD(lParam) & KF_EXTENDED))
                    wParam = IM_VK_KEYPAD_ENTER;
                // Submit key event
                const bool is_key_down = (message == WM_KEYDOWN || message == WM_SYSKEYDOWN);
                const ImGuiKey key = ImGui_ImplWin32_VirtualKeyToImGuiKey(wParam);
                const int scancode = (int)LOBYTE(HIWORD(lParam));
                if (key != ImGuiKey_None)
                {
                    io.AddKeyEvent(key, is_key_down);
                    // To support legacy indexing (<1.87 user code)
                    io.SetKeyEventNativeData(key, wParam, scancode);
                }
            }
            break;
        }
        case WM_CHAR:
            if (IsWindowUnicode(_hWnd))
            {
                // You can also use ToAscii()+GetKeyboardState() to retrieve characters.
                if (wParam > 0 && wParam < 0x10000)
                    io.AddInputCharacterUTF16((unsigned short)wParam);
            }
            else
            {
                wchar_t wch = 0;
                ::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (char*)&wParam, 1, &wch, 1);
                io.AddInputCharacter(wch);
            }
            break;
        case WM_MOUSEWHEEL:
            io.AddMouseWheelEvent(0.0f, (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA);
            break;
        case WM_MOUSEHWHEEL:
            io.AddMouseWheelEvent((float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA, 0.0f);
            break;
            // return if it was an user input event, countinue to process if is system event.
        default: goto sys;
        }
        return 0;
    sys: 
        switch (message)
        {
        case WM_SETFOCUS:
        case WM_KILLFOCUS:
            io.AddFocusEvent(message == WM_SETFOCUS);
            break;
        }
    };

    switch (message)
    {
        // case WM_CREATE:
    case WM_PAINT:
    {
        //if drawing context is initialized
        const double delta = DeltaTime.now() - prevouis_time;
        if (delta*1000.0 > SleepLenght)
            if (ImGui::GetCurrentContext() && render_mutex.try_lock())
            {
                auto& io = ImGui::GetCurrentContext()->IO;
                io.DeltaTime = delta;
                io.DisplaySize = ImVec2(width,height);
                prevouis_time += delta;
                ImGui_ImplOpenGL3_NewFrame();
                ImGui::NewFrame();
                if (rc.OnDraw(delta, true))
                    isRunning = 0;
                //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                ImGui::Render();
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
                SwapBuffers(hDC);
                render_mutex.unlock();
            }
    }
        break;
    case WM_QUIT:
        isRunning = 0;
        break;
    case WM_DESTROY:
        break;
    case WM_CLOSE:
        // like PostMessage,
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(_hWnd, message, wParam, lParam);
    }
    return 0;
}



ImGuiKey ImGui_ImplWin32_VirtualKeyToImGuiKey(WPARAM wParam)
{
    if (wParam >= 0x41 && wParam <= 0x5A) {
        return (ImGuiKey)(ImGuiKey_A + (wParam - 0x41));

    }
    if (wParam >= 0x30 && wParam <= 0x39) {
        return (ImGuiKey)(ImGuiKey_0 + (wParam - 48));
    }
    if (wParam >= ImGuiKey_Keypad0 && wParam <= ImGuiKey_Keypad9) {
        return (ImGuiKey)(ImGuiKey_Keypad0 + (wParam - ImGuiKey_Keypad0));
    }
    if (wParam >= VK_F1 && wParam <= VK_F12) {
        return (ImGuiKey)(ImGuiKey_F1 + (wParam - VK_F1));
    }
    switch (wParam)
    {
    case VK_ESCAPE: return ImGuiKey_Escape;
    case VK_OEM_MINUS: return ImGuiKey_Minus;
    case VK_OEM_NEC_EQUAL: return ImGuiKey_Equal;
    case VK_BACK: return ImGuiKey_Backspace;
    case VK_TAB: return ImGuiKey_Tab;
    case VK_OEM_4: return ImGuiKey_LeftBracket;
    case VK_OEM_6: return ImGuiKey_RightBracket;
    case VK_RETURN: return ImGuiKey_Enter;
    case VK_OEM_1: return ImGuiKey_Semicolon;
    case VK_OEM_COMMA: return ImGuiKey_Comma;
    case VK_OEM_3: return ImGuiKey_GraveAccent;
    case VK_OEM_5: return ImGuiKey_Backslash;
    case VK_OEM_7: return ImGuiKey_Apostrophe;
    case VK_OEM_PERIOD: return ImGuiKey_Period;
    case VK_OEM_2: return ImGuiKey_Slash;
    case VK_MULTIPLY: return ImGuiKey_KeypadMultiply;
    case VK_SPACE: return ImGuiKey_Space;
    case VK_CAPITAL: return ImGuiKey_CapsLock;
    case VK_NUMLOCK:        return ImGuiKey_NumLock;
    case VK_SUBTRACT:        return ImGuiKey_KeypadSubtract;
    case VK_ADD:        return ImGuiKey_KeypadAdd;
    case VK_DECIMAL:        return ImGuiKey_KeypadDecimal;
    case IM_VK_KEYPAD_ENTER: return ImGuiKey_KeypadEnter;
    case VK_DIVIDE: return ImGuiKey_KeypadDivide;
    case VK_HOME: return ImGuiKey_Home;
    case VK_UP: return ImGuiKey_UpArrow;
    case VK_LEFT: return ImGuiKey_LeftArrow;
    case VK_NAVIGATION_UP: return ImGuiKey_PageUp;
    case VK_RIGHT: return ImGuiKey_RightArrow;
    case VK_END: return ImGuiKey_End;
    case VK_DOWN: return ImGuiKey_DownArrow;
    case VK_NAVIGATION_DOWN: return ImGuiKey_PageDown;
    case VK_INSERT: return ImGuiKey_Insert;
    case VK_DELETE: return ImGuiKey_Delete;
    default:
        //printf("key: %i\n",keycode);
        return ImGuiKey_None;
    }
}
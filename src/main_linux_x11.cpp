/*------------------------------------------------------------------------
 * A demonstration of OpenGL in a  ARGB window
 *    => support for composited window transparency
 *
 * (c) 2011 by Wolfgang 'datenwolf' Draxinger
 *     See me at comp.graphics.api.opengl and StackOverflow.com

 * License agreement: This source code is provided "as is". You
 * can use this source code however you want for your own personal
 * use. If you give this source code to anybody else then you must
 * leave this message in it.
 *
 * This program is based on the simplest possible
 * Linux OpenGL program by FTB (see info below)

  The simplest possible Linux OpenGL program? Maybe...

  (c) 2002 by FTB. See me in comp.graphics.api.opengl

  --
  <\___/>
  / O O \
  \_____/  FTB.

------------------------------------------------------------------------*/
//#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <sys/types.h>
#include <time.h>

#include "glcorearb.h"
#include <GL/glx.h>
#include <GL/glxext.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrender.h>
#include <X11/Xutil.h>
// X11 is a real jackass!
#undef None
#undef Bool
#undef Always
static constexpr int None=0;
typedef int Bool;



#include "Window.h"
#include "Core.hpp"
#include "Exception.hpp"
#include "Timer.hpp"
#include <unistd.h>

#include "imgui/imgui.h"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui/imgui_internal.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_my.hpp"


static bool isRunning=true;
static int width, height;
static Atom del_atom;
static Core rc;

static const int VisData[] = {
GLX_RENDER_TYPE, GLX_RGBA_BIT,
GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
GLX_DOUBLEBUFFER, True,
GLX_RED_SIZE, 8,
GLX_GREEN_SIZE, 8,
GLX_BLUE_SIZE, 8,
GLX_ALPHA_SIZE, 8,
GLX_DEPTH_SIZE, 16,
//
GLX_X_RENDERABLE    , True,
GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
GLX_SAMPLE_BUFFERS  , 1,
GLX_SAMPLES         , 4,
//
None
};

#define GLX_CONTEXT_MAJOR_VERSION_ARB       0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB       0x2092
static const int context_attribs[] =
{
	GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
	GLX_CONTEXT_MINOR_VERSION_ARB, 2,
	//GLX_CONTEXT_FLAGS_ARB        , GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
	None
};

static Bool WaitForMapNotify(Display *d, XEvent *e, char *arg)
{
	return d && e && arg && (e->type == MapNotify) && (e->xmap.window == *(Window*)arg);
}


static int ctxErrorHandler( Display *dpy, XErrorEvent *ev )
{
    (void)dpy;(void)ev;
    fputs("Error at context creation\n", stderr);
    return 0;
}



static int WndProc(XEvent *event);

int main(int argc, char *argv[])
{
    (void)argc;(void)argv;
    XEvent event;
	// frame len calculated by deviding a second by expected number of frames in a second
	constexpr double frame_len = 1000.0/30.0;

	ImGui_ImplMy_Data* bd;
	Display *Xdisplay;
	Window window_handle;
	Colormap cmap;
	GLXContext render_context;
	Timer DeltaTime;

    DeltaTime.init();
	{
		GLXFBConfig fbconfig;
		{
			int x,y, attr_mask;
			XSizeHints hints;
			XWMHints *startup_state;
			XTextProperty textprop;
			XSetWindowAttributes attr;
			XVisualInfo *visual;
			Window Xroot;
			int Xscreen;
			GLXFBConfig *fbconfigs;
			int numfbconfigs;
			const char *title = "ImGUI";

			Xdisplay = XOpenDisplay(NULL);
			if (!Xdisplay)
				throw Exception("Couldn't connect to X server\n");
			Xscreen = DefaultScreen(Xdisplay);
			Xroot = RootWindow(Xdisplay, Xscreen);

			fbconfigs = glXChooseFBConfig(Xdisplay, Xscreen, VisData, &numfbconfigs);
			fbconfig = 0;
			for(int i = 0; i<numfbconfigs; i++) {
				XRenderPictFormat *pict_format;
				visual = (XVisualInfo*) glXGetVisualFromFBConfig(Xdisplay, fbconfigs[i]);
				if(!visual)
					continue;

				pict_format = XRenderFindVisualFormat(Xdisplay, visual->visual);
				if(!pict_format)
					goto jumpFree;

				fbconfig = fbconfigs[i];
				if(pict_format->direct.alphaMask > 0) {
					break;
				}
				jumpFree:
				XFree(visual);
			}
			XFree( fbconfigs );

			if(!fbconfig)
				throw Exception("No matching FB config found");

			//in the case you might ask: glXGetFBConfigAttrib(Xdisplay, fbconfig, GLX_DOUBLEBUFFER, &doublebuffer);

			/* Create a colormap - only needed on some X clients, eg. IRIX */
			cmap = XCreateColormap(Xdisplay, Xroot, visual->visual, AllocNone);

			attr.colormap = cmap;
			attr.background_pixmap = None;
			attr.border_pixmap = None;
			attr.border_pixel = 0;
			attr.event_mask = EnterWindowMask | LeaveWindowMask | OwnerGrabButtonMask |ExposureMask | VisibilityChangeMask | StructureNotifyMask | PropertyChangeMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | ButtonMotionMask | PointerMotionMask;

			attr_mask =
			//	CWBackPixmap|
				CWColormap|
				CWBorderPixel|
				CWEventMask;

			width = DisplayWidth(Xdisplay, Xscreen)/2;
			height = DisplayHeight(Xdisplay, Xscreen)/2;
			x=width/2, y=height/2;

			window_handle = XCreateWindow(	Xdisplay,
							Xroot,
							x, y, width, height,
							0,
							visual->depth,
							InputOutput,
							visual->visual,
							attr_mask, &attr);
			if( !window_handle )
				throw Exception("Couldn't create the window\n");
			XFree(visual);
			XSetStandardProperties(Xdisplay, window_handle, "xogl", "xogl", None, argv, argc, NULL); 

			textprop.value = (unsigned char*)title;
			textprop.encoding = XA_STRING;
			textprop.format = 8;
			textprop.nitems = strlen(title);

			hints.x = x;
			hints.y = y;
			hints.width = width;
			hints.height = height;
			hints.flags = USPosition|USSize;

			startup_state = XAllocWMHints();
			startup_state->initial_state = NormalState;
			startup_state->flags = StateHint;

			XSetWMProperties(Xdisplay, window_handle,&textprop, &textprop,
					NULL, 0,
					&hints,
					startup_state,
					NULL);

			XFree(startup_state);

			XMapWindow(Xdisplay, window_handle);
			XIfEvent(Xdisplay, &event, WaitForMapNotify, (char*)&window_handle);

			if ((del_atom = XInternAtom(Xdisplay, "WM_DELETE_WINDOW", 0)) != None) {
				XSetWMProtocols(Xdisplay, window_handle, &del_atom, 1);
			}
		}
		rc.OnInit();
		{
			int dummy;
			if (!glXQueryExtension(Xdisplay, &dummy, &dummy))
				throw Exception("OpenGL not supported by X server\n");

			render_context = NULL;
			{
				typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
				glXCreateContextAttribsARBProc glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc)glXGetProcAddressARB((unsigned char*)"glXCreateContextAttribsARB" );
				if( glXCreateContextAttribsARB ) {

					int (*oldHandler)(Display*, XErrorEvent*) = XSetErrorHandler(&ctxErrorHandler);

					render_context = glXCreateContextAttribsARB( Xdisplay, fbconfig, 0, True, context_attribs );

					XSync( Xdisplay, False );
					XSetErrorHandler( oldHandler );
				} else
					throw Exception("glXCreateContextAttribsARB could not be retrieved\n");
			}

			if(!render_context)
			{
				fputs("using fallback\n", stderr);

				render_context = glXCreateNewContext(Xdisplay, fbconfig, GLX_RGBA_TYPE, 0, True);
				if (!render_context)
					throw Exception("Failed to create a GL context\n");
			}

			if (!glXMakeContextCurrent(Xdisplay, window_handle, window_handle, render_context))
				throw Exception("glXMakeCurrent failed for window\n");

			glwInitialize(0x304);
		}
	}


    #if !defined(_WIN32)
    /* Avoid locale-related number parsing issues */
    setlocale(LC_NUMERIC, "C");
    #endif
    rc.OnCreate();


    ImGui::CreateContext();
    ImGuiIO& io=ImGui::GetIO();
    bd = IM_NEW(ImGui_ImplMy_Data)();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable some options
    IM_ASSERT(io.BackendPlatformUserData == nullptr && "Already initialized a platform backend!");
    io.BackendPlatformUserData = (void*)bd;
    io.BackendPlatformName = "imgui_impl_my";
    io.DisplaySize = ImVec2(width, height);
    ImGui::GetMainViewport()->PlatformHandleRaw = (void*)window_handle;
    ImGui_ImplOpenGL3_Init();
    ImGui::StyleColorsLight();


	// Create a nanogui screen
    glClearColor(0,0,0,0.5);
    glClearDepth(0.0);
    rc.OnStart();

    DeltaTime.start_point();

    while (isRunning) {
        if(XPending(Xdisplay)){
            XNextEvent(Xdisplay, &event);
            WndProc(&event);
        }else{
            io.DeltaTime = DeltaTime.dtime/1000.0;
            // Beginning of frame: update Renderer + Platform backend, start Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui::NewFrame();

            if(rc.OnDraw(DeltaTime.dtime,true))
                isRunning=0;

            glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glXSwapBuffers(Xdisplay, window_handle);
            DeltaTime.end_point();
	        if(DeltaTime.dtime < frame_len)
	        {
		        usleep((int) (frame_len - DeltaTime.dtime)*1000);
		        DeltaTime.dtime = frame_len;
	        }
            DeltaTime.start_point();
        }
    }
	rc.OnStop();
	ImGui_ImplOpenGL3_Shutdown();
	rc.OnDestroy();
	IM_DELETE(bd);
	ImGui::DestroyContext();
	glwDestroy();

    glXDestroyContext( Xdisplay, render_context );

    XDestroyWindow( Xdisplay, window_handle );
	XFreeColormap( Xdisplay, cmap );

    XCloseDisplay( Xdisplay );

	return 0;
}




static ImGuiKey ImGui_ImplLinux_VirtualKeyToImGuiKey(unsigned int);
static ImGuiKey ImGui_ImplLinux_VirtualKeyToImGuiModKey(unsigned int keycode);
int WndProc (XEvent *event)
{
	//if(event->type != 22)
	// Console(Logger::developer,"Event: %d\n",event->type);
	ImGuiIO& io=ImGui::GetIO();
	KeySym unic;
	switch (event->type)
	{
	case ConfigureNotify:
		io.DisplaySize = ImVec2((float)event->xconfigure.width, (float)event->xconfigure.height);
        //Console(Tiz::Logger::developer,"Resize: %d %d\n",event->xconfigure.width,event->xconfigure.height);
		break;
	case KeyPress:
		// Tiz::Console(Tiz::Logger::debug,"press: %d\n",event->xkey.keycode);
		io.AddKeyEvent(ImGui_ImplLinux_VirtualKeyToImGuiModKey(event->xkey.keycode), 1);
		io.AddKeyEvent(ImGui_ImplLinux_VirtualKeyToImGuiKey(event->xkey.keycode), true);
		unic=XLookupKeysym(&event->xkey, 0);

		//putchar(unic);Console.flush();
		if ((0x20<=unic && unic<=0x7e)
		|| (0xa0<=unic && unic<=0xff)
		|| (0x1a1<=unic && unic<=0x1ff)
		|| (0x2a1<=unic && unic<=0x2fe)
		){
			io.AddInputCharacter(unic);
		}
		break;
	case KeyRelease:
		//XMaskEvent()
		io.AddKeyEvent(ImGui_ImplLinux_VirtualKeyToImGuiModKey(event->xkey.keycode), 0);
		io.AddKeyEvent(ImGui_ImplLinux_VirtualKeyToImGuiKey(event->xkey.keycode), false);
		break;
	/*
		1 = left button
		2 = middle button (pressing the scroll wheel)
		3 = right button
		4 = turn scroll wheel up
		5 = turn scroll wheel down
		6 = push scroll wheel left
		7 = push scroll wheel right
		8 = 4th button (aka browser backward button)
		9 = 5th button (aka browser forward button)
	*/
	case ButtonPress:
		if (Button1==event->xbutton.button)
			io.AddMouseButtonEvent(0, true);
        if (Button2==event->xbutton.button)
			io.AddMouseButtonEvent(2, true);
        if (Button3==event->xbutton.button)
			io.AddMouseButtonEvent(1, true);
		else if (event->xbutton.button==Button4)
			io.AddMouseWheelEvent(0.0f, 0.5);
		else if (event->xbutton.button==Button5)
			io.AddMouseWheelEvent(0.0f,-0.5);
		break;
	case ButtonRelease:
		if (Button1==event->xbutton.button)
			io.AddMouseButtonEvent(0, false);
        if (Button2==event->xbutton.button)
			io.AddMouseButtonEvent(2, false);
        if (Button3==event->xbutton.button)
			io.AddMouseButtonEvent(1, false);
		else if (event->xbutton.button==Button4)
			io.AddMouseWheelEvent(0.0f, 0.5);
		else if (event->xbutton.button==Button5)
			io.AddMouseWheelEvent(0.0f,-0.5);
		break;
	case MotionNotify:
		io.AddMousePosEvent((float)event->xmotion.x,(float)event->xmotion.y);
		break;
	case Expose:
		break;
	case ClientMessage:
		if ((unsigned)(event->xclient.data.l[0]) == del_atom)
			isRunning = false;
		break;
	default:
		return 0;
	}
	return 1;
}






ImGuiKey ImGui_ImplLinux_VirtualKeyToImGuiModKey(unsigned int keycode)
{
    switch (keycode)
	{
	case 37:
	case 105:
        return ImGuiMod_Ctrl;
	case 64:
	case 108:
        return ImGuiMod_Alt;
	case 50:
	case 62:
        return ImGuiMod_Shift;
    default:
        return ImGuiKey_None;
    }
}
ImGuiKey ImGui_ImplLinux_VirtualKeyToImGuiKey(unsigned int keycode)
{
	switch (keycode)
	{
	//ImGuiKey_0
	//case XK_0 ... XK_9: return (ImGuiKey)key+488;


	case 9: return ImGuiKey_Escape;
	case 10 ... 18: return (ImGuiKey)(ImGuiKey_1+(keycode-10));
	case 19: return ImGuiKey_0;
	case 20: return ImGuiKey_Minus;
	case 21: return ImGuiKey_Equal;
	case 22: return ImGuiKey_Backspace;


	case 23: return ImGuiKey_Tab;
	case 24: return ImGuiKey_Q;
	case 25: return ImGuiKey_W;
	case 26: return ImGuiKey_E;
	case 27: return ImGuiKey_R;
	case 28: return ImGuiKey_T;
	case 29: return ImGuiKey_Y;
	case 30: return ImGuiKey_U;
	case 31: return ImGuiKey_I;
	case 32: return ImGuiKey_O;
	case 33: return ImGuiKey_P;
	case 34: return ImGuiKey_LeftBracket;
	case 35: return ImGuiKey_RightBracket;

	case 36: return ImGuiKey_Enter;
	case 37: return ImGuiKey_LeftCtrl;
	case 38: return ImGuiKey_A;
	case 39: return ImGuiKey_S;
	case 40: return ImGuiKey_D;
	case 41: return ImGuiKey_F;
	case 42: return ImGuiKey_G;
	case 43: return ImGuiKey_H;
	case 44: return ImGuiKey_J;
	case 45: return ImGuiKey_K;
	case 46: return ImGuiKey_L;
	case 47: return ImGuiKey_Semicolon;
	case 48: return ImGuiKey_Comma;
	case 49: return ImGuiKey_GraveAccent;

	case 50: return ImGuiKey_LeftShift;
	case 51: return ImGuiKey_Backslash;
	case 52: return ImGuiKey_Z;
	case 53: return ImGuiKey_X;
	case 54: return ImGuiKey_C;
	case 55: return ImGuiKey_V;
	case 56: return ImGuiKey_B;
	case 57: return ImGuiKey_N;
	case 58: return ImGuiKey_M;
	case 59: return ImGuiKey_Apostrophe;
	case 60: return ImGuiKey_Period;
	case 61: return ImGuiKey_Slash;
	case 62: return ImGuiKey_RightShift;
	case 63: return ImGuiKey_KeypadMultiply;
	case 64: return ImGuiKey_LeftAlt;
	case 65: return ImGuiKey_Space;
	case 66: return ImGuiKey_CapsLock;

	case 67 ... 76: return (ImGuiKey)(ImGuiKey_F1+(keycode-67));


	case 77:        return ImGuiKey_NumLock;
	case 79 ... 81: return (ImGuiKey)(ImGuiKey_Keypad7+(keycode-79));
	case 82:        return ImGuiKey_KeypadSubtract;
	case 83 ... 85: return (ImGuiKey)(ImGuiKey_Keypad4+(keycode-83));
	case 86:        return ImGuiKey_KeypadAdd;
	case 87 ... 89: return (ImGuiKey)(ImGuiKey_Keypad1+(keycode-87));
	case 90:        return ImGuiKey_Keypad0;
	case 91:        return ImGuiKey_KeypadDecimal;


	case 95: return ImGuiKey_F11;
	case 96: return ImGuiKey_F12;

	case 104: return ImGuiKey_KeypadEnter;
	case 105: return ImGuiKey_RightCtrl;
	case 106: return ImGuiKey_KeypadDivide;
	case 108: return ImGuiKey_RightAlt;

	case 110: return ImGuiKey_Home;
	case 111: return ImGuiKey_UpArrow;
	case 113: return ImGuiKey_LeftArrow;
	case 112: return ImGuiKey_PageUp;
	case 114: return ImGuiKey_RightArrow;
	case 115: return ImGuiKey_End;
	case 116: return ImGuiKey_DownArrow;
	case 117: return ImGuiKey_PageDown;
	case 118: return ImGuiKey_Insert;
	case 119: return ImGuiKey_Delete;
	default:
	//printf("key: %i\n",keycode);
	return ImGuiKey_None;
	}
}

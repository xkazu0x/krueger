#ifndef KRUEGER_OPENGL_WIN32_H
#define KRUEGER_OPENGL_WIN32_H

#include <gl/gl.h>
#pragma comment(lib, "opengl32")

#define WGL_DRAW_TO_WINDOW_ARB            0x2001
#define WGL_SUPPORT_OPENGL_ARB            0x2010
#define WGL_DOUBLE_BUFFER_ARB             0x2011
#define WGL_PIXEL_TYPE_ARB                0x2013
#define WGL_COLOR_BITS_ARB                0x2014
#define WGL_DEPTH_BITS_ARB                0x2022
#define WGL_STENCIL_BITS_ARB              0x2023
#define WGL_TYPE_RGBA_ARB                 0x202B

#define WGL_CONTEXT_MAJOR_VERSION_ARB     0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB     0x2092

#define WGL_PROC_LIST \
  WGL_PROC(wglChoosePixelFormatARB, BOOL, (HDC hDC, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats)) \
  WGL_PROC(wglCreateContextAttribsARB, HGLRC, (HDC hDC, HGLRC hShareContext, const int *attribList))

#define WGL_PROC(name, r, p) typedef r name##_proc p;
WGL_PROC_LIST
#undef WGL_PROC

#define WGL_PROC(name, r, p) global name##_proc *name;
WGL_PROC_LIST
#undef WGL_PROC

global const int _wgl_pf_attribs[] = {
  WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
  WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
  WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
  WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
  WGL_COLOR_BITS_ARB, 32,
  WGL_DEPTH_BITS_ARB, 24,
  WGL_STENCIL_BITS_ARB, 8,
  0
};

global HGLRC _gl_win32_ctx;

#endif // KRUEGER_OPENGL_WIN32_H

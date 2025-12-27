#ifndef KRUEGER_OPENGL_LINUX_H
#define KRUEGER_OPENGL_LINUX_H

#include <GL/gl.h>
#include <GL/glx.h>

#define GLX_CONTEXT_MAJOR_VERSION_ARB          0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB          0x2092
#define GLX_CONTEXT_FLAGS_ARB                  0x2094

#define GLX_CONTEXT_DEBUG_BIT_ARB              0x00000001
#define GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB 0x00000002

typedef GLXContext (*glXCreateContextAttribsARB_Proc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);

global GLXContext _ogl_lnx_ctx;

#endif // KRUEGER_OPENGL_LINUX_H

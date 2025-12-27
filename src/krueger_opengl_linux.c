#ifndef KRUEGER_OPENGL_LINUX_C
#define KRUEGER_OPENGL_LINUX_C

internal void *
ogl_load_proc(char *name) {
  void *result = (void *)glXGetProcAddressARB((u8 *)name);
  return(result);
}

internal void
ogl_init(void) {
  int glx_version_major = 0;
  int glx_version_minor = 0;
  if(!glXQueryVersion(_lnx_gfx_state->display, &glx_version_major, &glx_version_minor) ||
     (glx_version_major == 1 && glx_version_minor < 3) ||
     glx_version_major < 1) {
    log_error("OpenGL: Unsupported GLX version (%i.%i, need at least 1.3)", glx_version_major, glx_version_minor);
    platform_abort(1);
  }
  
  local int fb_cfg_attribs[] = {
    GLX_X_RENDERABLE,   True,
    GLX_DRAWABLE_TYPE,  GLX_WINDOW_BIT,
    GLX_RENDER_TYPE,    GLX_RGBA_BIT,
    GLX_X_VISUAL_TYPE,  GLX_TRUE_COLOR,
    GLX_RED_SIZE,       8,
    GLX_GREEN_SIZE,     8,
    GLX_BLUE_SIZE,      8,
    GLX_ALPHA_SIZE,     8,
    GLX_DEPTH_SIZE,     24,
    GLX_STENCIL_SIZE,   8,
    GLX_DOUBLEBUFFER,   True,
    None
  };

  int fb_cfg_count = 0;
  GLXFBConfig *fb_cfgs = glXChooseFBConfig(_lnx_gfx_state->display, DefaultScreen(_lnx_gfx_state->display), fb_cfg_attribs, &fb_cfg_count);
  if(!fb_cfgs) {
    log_error("OpenGL: Could not find a suitable framebuffer configuration.");
    platform_abort(1);
  }

  GLXFBConfig fb_cfg = fb_cfgs[0];
  XFree(fb_cfgs);
  
  glXCreateContextAttribsARB_Proc glXCreateContextAttribsARB = 0;
  glXCreateContextAttribsARB = (glXCreateContextAttribsARB_Proc)glXGetProcAddressARB((u8 *)"glXCreateContextAttribsARB");

  int ctx_attribs[] = {
    GLX_CONTEXT_MAJOR_VERSION_ARB, OGL_MAJOR_VER,
    GLX_CONTEXT_MINOR_VERSION_ARB, OGL_MINOR_VER,
    None
  };

  _ogl_lnx_ctx = glXCreateContextAttribsARB(_lnx_gfx_state->display, fb_cfg, 0, true, ctx_attribs);
  glXMakeCurrent(_lnx_gfx_state->display, 0, _ogl_lnx_ctx);
}

internal void
ogl_window_equip(Platform_Handle handle) {
}

internal void
ogl_window_select(Platform_Handle handle) {
  if (platform_handle_is_valid(handle)) {
    _Linux_Window *window = _linux_window_from_handle(handle);
    glXMakeCurrent(_lnx_gfx_state->display, window->xwnd, _ogl_lnx_ctx);
  }
}

internal void
ogl_window_swap(Platform_Handle handle) {
  if (platform_handle_is_valid(handle)) {
    _Linux_Window *window = _linux_window_from_handle(handle);
    glXSwapBuffers(_lnx_gfx_state->display, window->xwnd);
  }
}

#endif // KRUEGER_OPENGL_LINUX_C

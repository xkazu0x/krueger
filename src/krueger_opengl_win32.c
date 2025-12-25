#ifndef KRUEGER_OPENGL_WIN32_C
#define KRUEGER_OPENGL_WIN32_C

internal void
gl_init(void) {
  WNDCLASSEXW wndclass = {.cbSize = sizeof(wndclass)};
  wndclass.lpfnWndProc = DefWindowProcW;
  wndclass.hInstance = GetModuleHandleW(0);
  wndclass.lpszClassName = L"bootstrap_window";

  ATOM atom = RegisterClassExW(&wndclass);
  HWND hwnd = CreateWindowExW(0, MAKEINTATOM(atom), L"", 0,
                              CW_USEDEFAULT, CW_USEDEFAULT,
                              CW_USEDEFAULT, CW_USEDEFAULT,
                              0, 0, wndclass.hInstance, 0);

  HDC hdc = GetDC(hwnd);

  PIXELFORMATDESCRIPTOR pfd = {.nSize = sizeof(pfd)};
  pfd.nVersion = 1;
  pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
  pfd.iPixelType = PFD_TYPE_RGBA;
  pfd.cColorBits = 32;
  pfd.cDepthBits = 24;
  pfd.cStencilBits = 8;
  pfd.iLayerType = PFD_MAIN_PLANE;

  int pf = ChoosePixelFormat(hdc, &pfd);
  DescribePixelFormat(hdc, pf, sizeof(pfd), &pfd);
  SetPixelFormat(hdc, pf, &pfd);

  HGLRC hglrc = wglCreateContext(hdc);
  wglMakeCurrent(hdc, hglrc);

  wglChoosePixelFormatARB = (FNWGLCHOOSEPIXELFORMATARBPROC *)wglGetProcAddress("wglChoosePixelFormatARB");
  wglCreateContextAttribsARB = (FNWGLCREATECONTEXTATTRIBSARBPROC *)wglGetProcAddress("wglCreateContextAttribsARB");

  UINT num_formats = 0;
  wglChoosePixelFormatARB(hdc, _wgl_pf_attribs, 0, 1, &pf, &num_formats);

  _win32_gl_ctx = wglCreateContextAttribsARB(hdc, hglrc, _wgl_ctx_attribs);

  wglMakeCurrent(hdc, 0);
  wglDeleteContext(hglrc);
  ReleaseDC(hwnd, hdc);
  DestroyWindow(hwnd);
}

internal void
gl_window_equip(Platform_Handle handle) {
  if (platform_handle_is_valid(handle)) {
    Win32_Window *window = win32_window_from_handle(handle);

    int pf = 0;
    UINT num_formats = 0;
    wglChoosePixelFormatARB(window->hdc, _wgl_pf_attribs, 0, 1, &pf, &num_formats);

    PIXELFORMATDESCRIPTOR pfd;
    DescribePixelFormat(window->hdc, pf, sizeof(pfd), &pfd);
    SetPixelFormat(window->hdc, pf, &pfd);

    wglMakeCurrent(window->hdc, _win32_gl_ctx);
  }
}

internal void
gl_window_select(Platform_Handle handle) {
  if (platform_handle_is_valid(handle)) {
    Win32_Window *window = win32_window_from_handle(handle);
    wglMakeCurrent(window->hdc, _win32_gl_ctx);
  }
}

internal void
gl_window_swap(Platform_Handle handle) {
  if (platform_handle_is_valid(handle)) {
    Win32_Window *window = win32_window_from_handle(handle);
    SwapBuffers(window->hdc);
  }
}

#endif // KRUEGER_OPENGL_WIN32_C

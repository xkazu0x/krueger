#ifndef KRUEGER_BASE_LOG_C
#define KRUEGER_BASE_LOG_C

internal void
log_msg(Log_Type type, char *msg, ...) {
  char msg_buf[1024];
  char print_buf[2048];

  va_list arg_list;
  va_start(arg_list, msg);
  vsprintf_s(msg_buf, sizeof(msg_buf), msg, arg_list);
  va_end(arg_list);

  sprintf(print_buf, "%s %s\n", log_types_str[type], msg_buf);
  printf("%s", print_buf);
}

#endif // KRUEGER_BASE_LOG_C

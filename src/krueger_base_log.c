#ifndef KRUEGER_BASE_LOG_C
#define KRUEGER_BASE_LOG_C

internal void
log_msg(Log_Type type, char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  Temp scratch = scratch_begin(0, 0);
  String8 msg = str8_fmt_args(scratch.arena, fmt, args);
  String8 prefix = str8_cstr(log_type_table[type]);
  String8 out = str8_fmt(scratch.arena, "%s %s\n", prefix.str, msg.str);
  printf("%s", out.str);
  scratch_end(scratch);
  va_end(args);
}

#endif // KRUEGER_BASE_LOG_C

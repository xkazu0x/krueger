#ifndef KRUEGER_BASE_LOG_H
#define KRUEGER_BASE_LOG_H

typedef enum {
  LOG_INFO,
  LOG_ERROR,
  LOG_MAX,
} Log_Type;

global char *log_type_table[LOG_MAX] = {
  "[INFO]:",
  "[ERROR]:",
};

internal void log_msg(Log_Type type, char *fmt, ...);
#define log_info(fmt, ...) log_msg(LOG_INFO, fmt, ##__VA_ARGS__)
#define log_error(fmt, ...) log_msg(LOG_ERROR, fmt, ##__VA_ARGS__)

#endif // KRUEGER_BASE_LOG_H

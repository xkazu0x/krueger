#ifndef KRUEGER_BASE_LOG_H
#define KRUEGER_BASE_LOG_H

typedef enum {
  LOG_INFO,
  LOG_ERROR,
  LOG_MAX,
} Log_Type;

global char *log_types_str[LOG_MAX] = {
  "[INFO]:",
  "[ERROR]:",
};

#define log_info(msg, ...) log_msg(LOG_INFO, msg, ##__VA_ARGS__)
#define log_error(msg, ...) log_msg(LOG_ERROR, msg, ##__VA_ARGS__)

internal void log_msg(Log_Type type, char *msg, ...);

#endif // KRUEGER_BASE_LOG_H

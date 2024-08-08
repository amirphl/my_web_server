#ifndef LOG_H
#define LOG_H

#include "block_queue.h"
#include <cstdio>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <string>

class Log {
public:
  static Log *get_instance() {
    static Log instance;
    return &instance;
  }

  static void *flush_log_thread(void *args) {
    Log::get_instance()->async_write_log();
    return NULL;
  }

  bool init(const char *file_name, int close_log, int log_buf_size = 8192,
            int split_lines = 500000, int max_queue_size = 0);

  void write_log(int level, const char *format, ...);

  void flush(void);

private:
  Log();
  virtual ~Log();

  void *async_write_log() {
    std::string single_log;
    // TODO improve performance by using move assignment
    while (log_queue_->pop(single_log)) {
      mutex_.lock();
      fputs(single_log.c_str(), fp_);
      mutex_.unlock();
    }
    return NULL;
  }

  char dir_name_[128];
  char log_name_[128];
  int split_lines_;
  int log_buf_size_;
  long long count_;
  int today_;
  FILE *fp_;
  char *buf_;
  Block_queue<std::string> *log_queue_;
  bool is_async_;
  Locker mutex_;
  int close_log_;
};

#define LOG_DEBUG(format, ...)                                                 \
  if (close_log_ == 0) {                                                       \
    Log::get_instance()->write_log(0, format, ##__VA_ARGS__);                  \
    Log::get_instance()->flush();                                              \
  }
#define LOG_INFO(format, ...)                                                  \
  if (close_log_ == 0) {                                                       \
    Log::get_instance()->write_log(1, format, ##__VA_ARGS__);                  \
    Log::get_instance()->flush();                                              \
  }
#define LOG_WARN(format, ...)                                                  \
  if (close_log_ == 0) {                                                       \
    Log::get_instance()->write_log(2, format, ##__VA_ARGS__);                  \
    Log::get_instance()->flush();                                              \
  }
#define LOG_ERROR(format, ...)                                                 \
  if (close_log_ == 0) {                                                       \
    Log::get_instance()->write_log(3, format, ##__VA_ARGS__);                  \
    Log::get_instance()->flush();                                              \
  }
#endif

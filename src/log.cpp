#include "log.h"
#include "block_queue.h"
#include <cstdio>
#include <pthread.h>
#include <stdarg.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

Log::Log() {
  count_ = 0;
  is_async_ = false;
}

Log::~Log() {
  if (fp_ != NULL) {
    fclose(fp_);
  }
}

bool Log::init(const char *file_name, int close_log, int log_buf_size,
               int split_lines, int max_queue_size) {
  if (0 < max_queue_size) {
    is_async_ = true;
    log_queue_ = new Block_queue<std::string>(max_queue_size);
    pthread_t tid;
    pthread_create(&tid, NULL, flush_log_thread, NULL);
  }

  close_log_ = close_log;
  log_buf_size_ = log_buf_size;
  buf_ = new char[log_buf_size];
  memset(buf_, '\0', log_buf_size);
  split_lines_ = split_lines;

  time_t t = time(NULL);
  struct tm *sys_tm = localtime(&t);
  struct tm mytm = *sys_tm;

  const char *p = strrchr(file_name, '/');
  char log_full_name[256] = {0};

  if (p == NULL) {
    snprintf(log_full_name, 255, "%d_%02d_%02d_%s", mytm.tm_year + 1900,
             mytm.tm_mon + 1, mytm.tm_mday, file_name);
  } else {
    strcpy(log_name_, p + 1);
    strncpy(dir_name_, file_name, p - file_name + 1);
    snprintf(log_full_name, 255, "%s%d_%02d_%02d_%s", dir_name_,
             mytm.tm_year + 1900, mytm.tm_mon + 1, mytm.tm_mday, log_name_);
  }

  today_ = mytm.tm_mday;

  fp_ = fopen(log_full_name, "a");

  return fp_ != NULL;
}

void Log::write_log(int level, const char *format, ...) {
  struct timeval now = {0, 0};
  gettimeofday(&now, NULL);
  time_t t = now.tv_sec;
  struct tm *sys_tm = localtime(&t);
  struct tm mytm = *sys_tm;
  char s[16] = {0};

  switch (level) {
  case 0:
    strcpy(s, "[debug]:");
    break;
  case 1:
    strcpy(s, "[info]:");
    break;
  case 2:
    strcpy(s, "[warn]:");
    break;
  case 3:
    strcpy(s, "[error]:");
    break;
  default:
    strcpy(s, "[info]:");
    break;
  }

  mutex_.lock();
  count_++;

  if (today_ != mytm.tm_mday || count_ % split_lines_ == 0) {
    char new_log[256] = {0};
    fflush(fp_);
    fclose(fp_);
    char tail[16] = {0};

    snprintf(tail, 16, "%d_%02d_%02d_", mytm.tm_year + 1900, mytm.tm_mon + 1,
             mytm.tm_mday);

    if (today_ != mytm.tm_mday) {
      snprintf(new_log, 255, "%s%s%s", dir_name_, tail, log_name_);
      today_ = mytm.tm_mday;
      count_ = 0;
    } else {
      snprintf(new_log, 255, "%s%s%s.%lld", dir_name_, tail, log_name_,
               count_ / split_lines_);
    }

    fp_ = fopen(new_log, "a");
  }

  mutex_.unlock();

  va_list valst;
  va_start(valst, format);

  std::string log_str;
  mutex_.lock();

  int n = snprintf(buf_, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s ",
                   mytm.tm_year + 1900, mytm.tm_mon + 1, mytm.tm_mday,
                   mytm.tm_hour, mytm.tm_min, mytm.tm_sec, now.tv_usec, s);
  int m = vsnprintf(buf_ + n, log_buf_size_ - n - 1, format, valst);
  buf_[n + m] = '\n';
  buf_[n + m + 1] = '\0';
  log_str = buf_;

  mutex_.unlock();

  if (is_async_ && !log_queue_->full()) {
    log_queue_->push(log_str);
  } else {
    mutex_.lock();
    fputs(log_str.c_str(), fp_);
    mutex_.unlock();
  }

  va_end(valst);
}

void Log::flush(void) {
  mutex_.lock();
  fflush(fp_);
  mutex_.unlock();
}

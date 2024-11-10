#include "log.h"
#include "block_queue.h"
#include <cstdio>
#include <pthread.h>
#include <stdarg.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

Log::Log() {
  m_count = 0;
  m_is_async = false;
}

Log::~Log() {
  if (m_fp != NULL) {
    fclose(m_fp);
  }
}

bool Log::init(const char *file_name, int close_log, int log_buf_size,
               int split_lines, int max_queue_size) {
  if (0 < max_queue_size) {
    m_is_async = true;
    m_log_queue = new Block_queue<std::string>(max_queue_size);
    pthread_t tid;
    pthread_create(&tid, NULL, flush_log_thread, NULL);
  }

  m_close_log = close_log;
  m_log_buf_size = log_buf_size;
  m_buf = new char[log_buf_size];
  memset(m_buf, '\0', log_buf_size);
  m_split_lines = split_lines;

  time_t t = time(NULL);
  struct tm *sys_tm = localtime(&t);
  struct tm mytm = *sys_tm;

  const char *p = strrchr(file_name, '/');
  char log_full_name[256] = {0};

  if (p == NULL) {
    snprintf(log_full_name, 255, "%d_%02d_%02d_%s", mytm.tm_year + 1900,
             mytm.tm_mon + 1, mytm.tm_mday, file_name);
  } else {
    strcpy(m_log_name, p + 1);
    strncpy(m_dir_name, file_name, p - file_name + 1);
    snprintf(log_full_name, 255, "%s%d_%02d_%02d_%s", m_dir_name,
             mytm.tm_year + 1900, mytm.tm_mon + 1, mytm.tm_mday, m_log_name);
  }

  m_today = mytm.tm_mday;

  m_fp = fopen(log_full_name, "a");

  return m_fp != NULL;
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

  m_mutex.lock();
  m_count++;

  if (m_today != mytm.tm_mday || m_count % m_split_lines == 0) {
    char new_log[256] = {0};
    fflush(m_fp);
    fclose(m_fp);
    char tail[16] = {0};

    snprintf(tail, 16, "%d_%02d_%02d_", mytm.tm_year + 1900, mytm.tm_mon + 1,
             mytm.tm_mday);

    if (m_today != mytm.tm_mday) {
      snprintf(new_log, 255, "%s%s%s", m_dir_name, tail, m_log_name);
      m_today = mytm.tm_mday;
      m_count = 0;
    } else {
      snprintf(new_log, 255, "%s%s%s.%lld", m_dir_name, tail, m_log_name,
               m_count / m_split_lines);
    }

    m_fp = fopen(new_log, "a");
  }

  m_mutex.unlock();

  va_list valst;
  va_start(valst, format);

  std::string log_str;
  m_mutex.lock();

  int n = snprintf(m_buf, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s ",
                   mytm.tm_year + 1900, mytm.tm_mon + 1, mytm.tm_mday,
                   mytm.tm_hour, mytm.tm_min, mytm.tm_sec, now.tv_usec, s);
  int m = vsnprintf(m_buf + n, m_log_buf_size - n - 1, format, valst);
  m_buf[n + m] = '\n';
  m_buf[n + m + 1] = '\0';
  log_str = m_buf;

  m_mutex.unlock();

  if (m_is_async && !m_log_queue->full()) {
    m_log_queue->push(log_str);
  } else {
    m_mutex.lock();
    fputs(log_str.c_str(), m_fp);
    m_mutex.unlock();
  }

  va_end(valst);
}

void Log::flush(void) {
  m_mutex.lock();
  fflush(m_fp);
  m_mutex.unlock();
}

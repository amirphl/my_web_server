#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H

#include "locker.h"
#include <cstdint>
#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>

template <class T> class Block_queue {
public:
  Block_queue(int max_size = 1000) : m_max_size(max_size) {
    if (max_size <= 0) {
      exit(-1);
    }

    m_array = new T[max_size];
    m_front = m_back = -1;
  }

  ~Block_queue() {
    // TODO Destroyed concurrently by multiple threads?
    m_mutex.lock();
    if (m_array != NULL) {
      delete[] m_array;
    }
    m_mutex.unlock();
  }

  void clear() {
    m_mutex.lock();
    m_front = m_back = -1;
    m_mutex.unlock();
  }

  int size() {
    m_mutex.lock();
    int s;
    if (m_back == -1) {
      s = 0;
    } else if (m_back >= m_front) {
      s = m_back - m_front + 1;
    } else {
      s = m_max_size - (m_front - m_back - 1);
    }
    m_mutex.unlock();
    return s;
  }

  bool full() {
    m_mutex.lock();
    bool full = size() == m_max_size;
    m_mutex.unlock();
    return full;
  }

  bool empty() {
    m_mutex.lock();
    bool empty = size() == 0;
    m_mutex.unlock();
    return empty;
  }

  bool front(T &val) {
    m_mutex.lock();
    if (empty()) {
      m_mutex.unlock();
      return false;
    }
    val = m_array[m_front]; // TODO copy or move?
    m_mutex.unlock();
    return true;
  }

  bool back(T &val) {
    m_mutex.lock();
    if (full()) {
      m_mutex.unlock();
      return false;
    }
    val = m_array[m_back]; // TODO copy or move?
    m_mutex.unlock();
    return true;
  }

  bool push(const T &item) {
    m_mutex.lock();
    if (full()) {
      m_cond.broadcast(); // TODO What if waiting for consumer signal on cond?
      m_mutex.unlock();
      return false;
    }
    int64_t next = m_back; // 64bit for preventing overflow
    next = (next + 1) % m_max_size;
    m_array[next] = item; // TODO copy or move?
    m_back = next;
    if (m_front == -1) {
      m_front = m_back;
    }
    m_cond.broadcast();
    m_mutex.unlock();
    return true;
  }

  bool pop(T &item) {
    m_mutex.lock();
    while (empty()) {
      if (!m_cond.wait(m_mutex.get())) { // TODO
        m_mutex.unlock();
        return false;
      }
    }
    item = m_array[m_front];
    if (m_front == m_back) {
      m_front = m_back = -1;
    } else {
      int64_t next = m_front;
      next = (next + 1) % m_max_size;
      m_front = next;
    }
    m_mutex.unlock();
    return true;
  }

  bool pop(T &item, int ms_timeout) {
    struct timespec t = {0, 0};
    struct timeval now = {0, 0};
    gettimeofday(&now, NULL);
    m_mutex.lock();
    if (empty()) {
      t.tv_sec = now.tv_sec + ms_timeout / 1000;
      t.tv_nsec = (ms_timeout % 1000) * 1000;
      if (!m_cond.timewait(m_mutex.get(), t)) { // TODO
        m_mutex.unlock();
        return false;
      }
    }
    if (empty()) {
      m_mutex.unlock();
      return false;
    }
    item = m_array[m_front];
    if (m_front == m_back) {
      m_front = m_back = -1;
    } else {
      int64_t next = m_front;
      next = (next + 1) % m_max_size;
      m_front = next;
    }
    m_mutex.unlock();
    return true;
  }

private:
  Locker m_mutex;
  Cond m_cond;
  T *m_array;
  int m_front;
  int m_back;
  const int m_max_size;
};

#endif

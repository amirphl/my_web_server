#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H

#include "locker.h"
#include <cstdint>
#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>

template <class T> class Block_queue {
public:
  Block_queue(int max_size = 1000) : max_size_(max_size) {
    if (max_size <= 0) {
      exit(-1);
    }

    array_ = new T[max_size];
    front_ = back_ = -1;
  }

  ~Block_queue() {
    // TODO Destroyed concurrently by multiple threads?
    mutex_.lock();
    if (array_ != NULL) {
      delete[] array_;
    }
    mutex_.unlock();
  }

  void clear() {
    mutex_.lock();
    front_ = back_ = -1;
    mutex_.unlock();
  }

  int size() {
    mutex_.lock();
    int s;
    if (back_ == -1) {
      s = 0;
    } else if (back_ >= front_) {
      s = back_ - front_ + 1;
    } else {
      s = max_size_ - (front_ - back_ - 1);
    }
    mutex_.unlock();
    return s;
  }

  bool full() {
    mutex_.lock();
    bool full = size() == max_size_;
    mutex_.unlock();
    return full;
  }

  bool empty() {
    mutex_.lock();
    bool empty = size() == 0;
    mutex_.unlock();
    return empty;
  }

  bool front(T &val) {
    mutex_.lock();
    if (empty()) {
      mutex_.unlock();
      return false;
    }
    val = array_[front_]; // TODO copy or move?
    mutex_.unlock();
    return true;
  }

  bool back(T &val) {
    mutex_.lock();
    if (full()) {
      mutex_.unlock();
      return false;
    }
    val = array_[back_]; // TODO copy or move?
    mutex_.unlock();
    return true;
  }

  bool push(const T &item) {
    mutex_.lock();
    if (full()) {
      cond_.broadcast(); // TODO What if waiting for consumer signal on cond?
      mutex_.unlock();
      return false;
    }
    int64_t next = back_; // 64bit for preventing overflow
    next = (next + 1) % max_size_;
    array_[next] = item; // TODO copy or move?
    back_ = next;
    if (front_ == -1) {
      front_ = back_;
    }
    cond_.broadcast();
    mutex_.unlock();
    return true;
  }

  bool pop(T &item) {
    mutex_.lock();
    while (empty()) {
      if (!cond_.wait(mutex_.get())) { // TODO
        mutex_.unlock();
        return false;
      }
    }
    item = array_[front_];
    if (front_ == back_) {
      front_ = back_ = -1;
    } else {
      int64_t next = front_;
      next = (next + 1) % max_size_;
      front_ = next;
    }
    mutex_.unlock();
    return true;
  }

  bool pop(T &item, int ms_timeout) {
    struct timespec t = {0, 0};
    struct timeval now = {0, 0};
    gettimeofday(&now, NULL);
    mutex_.lock();
    if (empty()) {
      t.tv_sec = now.tv_sec + ms_timeout / 1000;
      t.tv_nsec = (ms_timeout % 1000) * 1000;
      if (!cond_.timewait(mutex_.get(), t)) { // TODO
        mutex_.unlock();
        return false;
      }
    }
    if (empty()) {
      mutex_.unlock();
      return false;
    }
    item = array_[front_];
    if (front_ == back_) {
      front_ = back_ = -1;
    } else {
      int64_t next = front_;
      next = (next + 1) % max_size_;
      front_ = next;
    }
    mutex_.unlock();
    return true;
  }

private:
  Locker mutex_;
  Cond cond_;
  T *array_;
  int front_;
  int back_;
  const int max_size_;
};

#endif

#ifndef LOCKER_H
#define LOCKER_H

#include <exception>
#include <pthread.h>
#include <semaphore.h>

class Sem {
public:
  Sem(int num = 0) {
    if (sem_init(&sem_, 0, num) != 0) {
      throw std::exception();
    }
  }
  ~Sem() { sem_destroy(&sem_); }
  bool wait() { return sem_wait(&sem_) == 0; }
  bool post() { return sem_post(&sem_) == 0; }

private:
  sem_t sem_;
};

class Locker {
public:
  Locker() {
    if (pthread_mutex_init(&mutex_, NULL) != 0) {
      throw std::exception();
    }
  }
  ~Locker() { pthread_mutex_destroy(&mutex_); }
  bool lock() { return pthread_mutex_lock(&mutex_) == 0; }
  bool unlock() { return pthread_mutex_unlock(&mutex_) == 0; }
  pthread_mutex_t *get() { return &mutex_; } // TODO del

private:
  pthread_mutex_t mutex_;
};

class Cond {
public:
  Cond() {
    if (pthread_cond_init(&m_cond, NULL) != 0) {
      throw std::exception();
    }
  }
  ~Cond() { pthread_cond_destroy(&m_cond); }
  bool wait(pthread_mutex_t *mutex) {
    return pthread_cond_wait(&m_cond, mutex) == 0;
  }
  bool timewait(pthread_mutex_t *mutex, struct timespec t) {
    return pthread_cond_timedwait(&m_cond, mutex, &t) == 0;
  }
  bool signal() { return pthread_cond_signal(&m_cond) == 0; }
  bool broadcast() { return pthread_cond_broadcast(&m_cond) == 0; }

private:
  pthread_cond_t m_cond;
};
#endif

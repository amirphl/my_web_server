#ifndef LST_TIMER
#define LST_TIMER

#include <arpa/inet.h>
#include <assert.h>
#include <csignal>
#include <netinet/in.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/event.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

class Util_Timer;

struct Client_Data {
  sockaddr_in addr;
  int sockfd;
  Util_Timer *timer;
};

class Util_Timer {
public:
  time_t m_expire;
  Util_Timer *m_prev;
  Util_Timer *m_next;
  Client_Data *m_user_data;
  void (*m_cb_func)(Client_Data *);

  Util_Timer() : m_prev(nullptr), m_next(nullptr), m_user_data(nullptr) {} // TODO init other fields
};

class Sort_Timer_Lst {
private:
  Util_Timer *m_head;
  Util_Timer *m_tail;
  void add_timer(Util_Timer *timer, Util_Timer *lst_head);

public:
  Sort_Timer_Lst() : m_head(nullptr), m_tail(nullptr) {}
  ~Sort_Timer_Lst();

  void add_timer(Util_Timer *timer);
  void adjust_timer(Util_Timer *timer);
  void del_timer(Util_Timer *timer);
  void tick();
};

class Utils {
public:
  static int *m_u_pipefd;
  static int u_kqfd;
  Sort_Timer_Lst *m_timer_lst;
  int m_timeslot;

  Utils();
  ~Utils();
  void init(int timeslot);
  void timer_handler() const;
  static int set_nonblocking(int fd);                                // NOLINT(readability-identifier-length)
  static void addfd(int kqfd, int fd, bool one_shot, int trig_mode); // NOLINT(readability-identifier-length)
  static void sig_handler(int sig);
  static void add_sig(int sig, void(handler)(int), bool restart = true);
  static void show_error(int connfd, const char *info);
};
#endif

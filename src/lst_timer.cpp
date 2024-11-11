#include "lst_timer.h"
#include "http_conn.h"

int *Utils::m_u_pipefd = 0;
int Utils::u_kqfd = 0;

Sort_Timer_Lst::~Sort_Timer_Lst() {
  Util_Timer *tmp = m_head;

  while (!tmp) { // NOLINT(readability-implicit-bool-conversion)
    m_head = tmp->m_next;
    delete tmp;
    tmp = m_head;
  }
}

void Sort_Timer_Lst::add_timer(Util_Timer *timer) {
  if (!timer) { // NOLINT(readability-implicit-bool-conversion)
    return;
  }

  if (!m_head) { // NOLINT(readability-implicit-bool-conversion)
    m_head = m_tail = timer;
    return;
  }

  if (timer->m_expire < m_head->m_expire) {
    timer->m_next = m_head;
    m_head->m_prev = timer;
    m_head = timer;
    return;
  }

  add_timer(timer, m_head);
}

void Sort_Timer_Lst::adjust_timer(Util_Timer *timer) {
  if (!timer) { // NOLINT(readability-implicit-bool-conversion)
    return;
  }

  Util_Timer *tmp = timer->m_next;
  if (!tmp || timer->m_expire < tmp->m_expire) { // NOLINT(readability-implicit-bool-conversion)
    return;
  }

  if (timer == m_head) {
    m_head = m_head->m_next;
    m_head->m_prev = nullptr;
    timer->m_next = nullptr;
    timer->m_prev = nullptr;
    add_timer(timer, m_head);
  } else if (timer == m_tail) {
    return;
  }

  timer->m_prev->m_next = timer->m_next;
  timer->m_next->m_prev = timer->m_prev;
  timer->m_prev = timer->m_next = nullptr;
  add_timer(timer, m_head);
}

void Sort_Timer_Lst::del_timer(Util_Timer *timer) {
  if (!timer) { // NOLINT(readability-implicit-bool-conversion)
    return;
  }

  if ((timer == m_head) && (timer == m_tail)) {
    delete timer;
    m_head = m_tail = nullptr;
    return;
  }

  if (timer == m_head) {
    m_head = m_head->m_next;
    m_head->m_prev = nullptr;
    delete timer;
    return;
  }

  if (timer == m_tail) {
    m_tail = m_tail->m_prev;
    m_tail->m_next = nullptr;
    delete timer;
    return;
  }

  timer->m_prev->m_next = timer->m_next;
  timer->m_next->m_prev = timer->m_prev;
  delete timer;
}

void Sort_Timer_Lst::add_timer(Util_Timer *timer, Util_Timer *lst_head) {
  if (!timer) { // NOLINT(readability-implicit-bool-conversion)
    return;
  }

  if (!lst_head) { // NOLINT(readability-implicit-bool-conversion)
    return;
  }

  Util_Timer *tmp = lst_head;
  while (tmp && tmp->m_expire < timer->m_expire) { // NOLINT(readability-implicit-bool-conversion)
    tmp = tmp->m_next;
  }

  if (!tmp) { // NOLINT(readability-implicit-bool-conversion)
    m_tail->m_next = timer;
    timer->m_prev = m_tail;
    m_tail = timer;
    return;
  }

  if (tmp == m_head) {
    timer->m_next = m_head;
    m_head->m_prev = timer;
    m_head = timer;
    return;
  }

  tmp->m_prev->m_next = timer;
  timer->m_prev = tmp->m_prev;
  timer->m_next = tmp;
  tmp->m_prev = timer;
}

void Sort_Timer_Lst::tick() {
  if (!m_head) { // NOLINT(readability-implicit-bool-conversion)
    return;
  }

  time_t curr = time(NULL);
  Util_Timer *tmp = m_head;

  while (tmp) { // NOLINT(readability-implicit-bool-conversion)
    if (curr < tmp->m_expire) {
      break;
    }

    tmp->m_cb_func(tmp->m_user_data);
    m_head = tmp->m_next;
    if (m_head) { // NOLINT(readability-implicit-bool-conversion)
      m_head->m_prev = nullptr;
    }

    delete tmp;
    tmp = m_head;
  }
}

void Utils::init(int timeslot) { m_timeslot = timeslot; }

int Utils::set_nonblocking(int fd) { // NOLINT(readability-identifier-length)
  int old_option = fcntl(fd, F_GETFL);
  int new_option = old_option | O_NONBLOCK;
  fcntl(fd, F_SETFL, new_option);
  return old_option;
}

// TODO Describe the purpose
void Utils::addfd(int kqfd, int fd, bool one_shot, int trig_mode) { // NOLINT(readability-identifier-length)
  struct kevent event;

  // For read events (equivalent to EPOLLIN)
  if (1 == trig_mode) {
    // Edge-triggered mode using NOTE_CLEAR | NOTE_EOF
    EV_SET(&event, fd, EVFILT_READ,
           EV_ADD | EV_ENABLE | EV_CLEAR, // Clear is similar to ET mode
           0,                             // Similar to EPOLLRDHUP
           0, NULL);
  } else {
    // Level-triggered mode (default)
    EV_SET(&event, fd, EVFILT_READ, EV_ADD | EV_ENABLE,
           0, // Similar to EPOLLRDHUP
           0, NULL);
  }

  // If one_shot is requested, add EV_ONESHOT flag
  if (one_shot) {
    event.flags |= EV_ONESHOT;
  }

  // Register the event
  if (kevent(kqfd, &event, 1, NULL, 0, NULL) == -1) {
    // Handle error
    perror("kevent register failed");
  }

  // Set the file descriptor to non-blocking mode
  set_nonblocking(fd);
}

// TODO Describe the purpose
void Utils::sig_handler(int sig) {
  int save_errno = errno;
  int msg = sig;
  send(m_u_pipefd[1], (char *)&msg, 1, 0);
  errno = save_errno;
}

// TODO Describe the purpose
void Utils::add_sig(int sig, void(handler)(int), bool restart) {
  struct sigaction sa;
  memset(&sa, '\0', sizeof(sa));
  sa.sa_handler = handler;

  if (restart) {
    sa.sa_flags |= SA_RESTART;
  }

  sigfillset(&sa.sa_mask);
  assert(sigaction(sig, &sa, NULL) != -1);
}

void Utils::timer_handler() const {
  m_timer_lst->tick();
  alarm(m_timeslot);
}

void Utils::show_error(int connfd, const char *info) {
  send(connfd, info, strlen(info), 0);
  close(connfd);
}

void cb_func(Client_Data *user_data) {
  struct kevent event;

  // Remove the file descriptor from kqueue
  EV_SET(&event, user_data->sockfd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
  kevent(Utils::u_kqfd, &event, 1, NULL, 0, NULL); // u_epollfd becomes u_kqfd

  assert(user_data);
  close(user_data->sockfd);
  Http_Conn::m_user_count--;
}

int create_kqueue() {
  int kqfd = kqueue();
  if (kqfd == -1) {
    perror("kqueue creation failed");

    return -1;
  }

  return kqfd;
}

// Example of how to wait for events (equivalent to epoll_wait)
static int wait_events(int kqfd, struct kevent *events, int maxevents, int timeout) {
  struct timespec ts;
  ts.tv_sec = timeout / 1000;
  ts.tv_nsec = (timeout % 1000) * 1000000;

  return kevent(kqfd, NULL, 0, events, maxevents, timeout >= 0 ? &ts : NULL);
}

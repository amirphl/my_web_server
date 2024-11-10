#ifndef _CONNECTION_POOL_
#define _CONNECTION_POOL_

#include "locker.h"
#include <list>
#include <mysql/jdbc.h>
#include <stdio.h>
#include <string>

class Connection_Pool {
public:
  std::string m_url;
  std::string m_port;
  std::string m_user;
  std::string m_password;
  std::string m_dbname;
  int m_close_log;

  sql::mysql::MySQL_Connection *get_connection();
  bool release_connection(sql::mysql::MySQL_Connection *);
  int num_free_conns() const;
  int num_current_conns() const;

  static Connection_Pool *get_instance();

  bool init(std::string url, std::string user, std::string password, std::string dbname, int port, int max_conn, int close_log);

private:
  Connection_Pool();
  ~Connection_Pool();

  int m_max_conn;
  int m_free_conn;
  Locker m_lock;
  std::list<sql::mysql::MySQL_Connection *> m_conns;
  Sem m_reserve;
};

class Connection_RAII {
public:
  Connection_RAII(sql::mysql::MySQL_Connection **conn, Connection_Pool *conn_pool);
  ~Connection_RAII();

private:
  sql::mysql::MySQL_Connection *m_conn_RAII;
  Connection_Pool *m_pool_RAII;
};
#endif

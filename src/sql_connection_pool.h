#ifndef _CONNECTION_POOL_
#define _CONNECTION_POOL_

#include "jdbc/mysql_connection.h"
#include "locker.h"
#include <list>
#include <mysql/jdbc.h>
#include <stdio.h>
#include <string>

class Connection_pool {
public:
  std::string url_;
  std::string port_;
  std::string user_;
  std::string password_;
  std::string dbname_;
  int close_log_;

  sql::mysql::MySQL_Connection *get_connection();
  bool release_connection(sql::mysql::MySQL_Connection *);
  int get_free_conn();
  void destory_pool();

  static Connection_pool *get_instance();

  void init(std::string url, std::string user, std::string password,
            std::string dbname, int port, int max_conn, int close_log);

private:
  Connection_pool();
  ~Connection_pool();

  int max_conn_;
  int cur_conn_;
  int free_conn_;
  Locker lock;
  std::list<sql::mysql::MySQL_Connection *> conns;
  Sem reserve;
};

class Connection_RAII {
public:
  Connection_RAII(sql::mysql::MySQL_Connection **conn,
                  Connection_pool *conn_pool);
  ~Connection_RAII();

private:
  sql::mysql::MySQL_Connection *connRAII;
  Connection_pool *poolRAII;
};
#endif

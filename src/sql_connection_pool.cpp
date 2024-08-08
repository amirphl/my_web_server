#include "sql_connection_pool.h"
#include <iostream>
#include <list>
#include <mysql/jdbc.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

Connection_pool::Connection_pool() {
  cur_conn_ = 0;
  free_conn_ = 0;
}

Connection_pool *Connection_pool::get_instance() {
  static Connection_pool conn_pool;
  return &conn_pool;
}

void Connection_pool::init(std::string url, std::string user,
                           std::string password, std::string)

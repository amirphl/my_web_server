// #include "sql_connection_pool.h"
// #include "include/jdbc/mysql_connection.h"

#include "sql_connection_pool.h"
#include "jdbc/cppconn/connection.h"
#include "jdbc/mysql_connection.h"
#include "log.h"
#include <exception>
#include <list>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

Connection_Pool::Connection_Pool() {
  m_max_conn = 0;
  m_free_conn = 0;
}

Connection_Pool *Connection_Pool::get_instance() {
  static Connection_Pool conn_pool; // NOTE static // TODO thread safe?
  return &conn_pool;
}

int Connection_Pool::num_current_conns() const { return m_max_conn - m_free_conn; }
int Connection_Pool::num_free_conns() const { return m_free_conn; }

// TODO thread safty
bool Connection_Pool::init(std::string url, std::string user, std::string password, std::string dbname, int port, int max_conn, int close_log) {
  m_lock.lock();

  m_url = url;
  m_user = user;
  m_password = password;
  m_dbname = dbname;
  m_port = std::to_string(port);
  m_max_conn = max_conn;
  m_close_log = close_log;

  try {
    for (auto i = 0; i < max_conn; ++i) {
      sql::Driver *driver = sql::mysql::get_driver_instance();
      // TODO SSL
      sql::Connection *base_conn = driver->connect("tcp://" + m_url + ":" + m_port, m_user, m_password);
      sql::mysql::MySQL_Connection *conn = (sql::mysql::MySQL_Connection *)(base_conn);

      if (conn == nullptr) {
        std::string msg = "MYSQL Connection Error";
        LOG_ERROR(msg.c_str());

        return false;
      }

      conn->setSchema(m_dbname);
      // TODO Set additional connection properties
      conn->setClientOption("OPT_CONNECT_TIMEOUT", "10");

      m_conns.push_back(conn);
      ++m_free_conn;
    }

    m_reserve = Sem(m_free_conn);
    m_max_conn = m_free_conn;
  } catch (const std::exception &e) {
    const std::string msg = "Connection Pool Init Error: " + std::string(e.what());
    LOG_ERROR(msg.c_str());
    m_lock.unlock();

    return false;
  }

  m_lock.unlock();

  return true;
}

sql::mysql::MySQL_Connection *Connection_Pool::get_connection() {
  if (m_conns.empty()) {
    return nullptr;
  }

  m_reserve.wait();
  m_lock.lock();
  sql::mysql::MySQL_Connection *conn = m_conns.front();
  m_conns.pop_front();
  --m_free_conn;
  m_lock.unlock();

  return conn;
}

bool Connection_Pool::release_connection(sql::mysql::MySQL_Connection *conn) {
  if (conn == nullptr) {
    return false;
  }

  m_lock.lock();
  m_conns.push_back(conn);
  ++m_free_conn;
  m_lock.unlock();
  m_reserve.post();

  return true;
}

Connection_Pool::~Connection_Pool() {
  m_lock.lock();

  for (auto it = m_conns.begin(); it != m_conns.end(); ++it) {
    sql::mysql::MySQL_Connection *conn = *it;
    try {
      conn->close();
      delete conn;
    } catch (sql::SQLException &e) {
      const std::string msg = "MySQL Close Error: " + std::string(e.what());
      LOG_ERROR(msg.c_str());
    }
  }

  m_free_conn = 0;
  m_conns.clear();

  m_lock.unlock();
}

Connection_RAII::Connection_RAII(sql::mysql::MySQL_Connection **conn, Connection_Pool *conn_pool) {
  *conn = conn_pool->get_connection();
  m_conn_RAII = *conn;
  m_pool_RAII = conn_pool;
}

Connection_RAII::~Connection_RAII() { m_pool_RAII->release_connection(m_conn_RAII); }

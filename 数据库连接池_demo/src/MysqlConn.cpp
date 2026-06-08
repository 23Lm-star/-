
#include <iostream>
#include "MysqlConn.h"

MysqlConn::MysqlConn(const std::string& host, int port,
                     const std::string& user, const std::string& password,
                     const std::string& dbname)
    : m_host(host), m_port(port), m_user(user), 
      m_password(password), m_dbname(dbname), m_conn(nullptr), m_isValid(false) {
}

MysqlConn::~MysqlConn() {
    disconnect();
}

bool MysqlConn::connect() {
    disconnect();
    
    m_conn = mysql_init(nullptr);
    if (!m_conn) {
        m_isValid.store(false);
        return false;
    }
    
    mysql_options(m_conn, MYSQL_SET_CHARSET_NAME, "utf8mb4");
    int timeout = 3;
    mysql_options(m_conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
    
    const char* timezone = "Asia/Shanghai";
    mysql_options(m_conn, MYSQL_INIT_COMMAND, "SET time_zone = '+08:00'");
    
    MYSQL* result = mysql_real_connect(m_conn, m_host.c_str(), m_user.c_str(),
                                       m_password.c_str(), m_dbname.c_str(),
                                       m_port, nullptr, 0);
    
    if (result) {
        m_isValid.store(true);
        updateLastUsedTime();
        return true;
    } else {
        std::cerr << "[ERROR] MySQL connection failed: " << mysql_error(m_conn) << std::endl;
        m_isValid.store(false);
        mysql_close(m_conn);
        m_conn = nullptr;
        return false;
    }
}

void MysqlConn::disconnect() {
    if (m_conn) {
        mysql_close(m_conn);
        m_conn = nullptr;
    }
    m_isValid.store(false);
}

bool MysqlConn::isValid() const {
    return m_isValid.load() && m_conn != nullptr;
}

bool MysqlConn::reconnect() {
    return connect();
}

void MysqlConn::markInvalid() {
    m_isValid.store(false);
}

bool MysqlConn::execute(const std::string& sql) {
    if (!isValid()) {
        return false;
    }
    
    updateLastUsedTime();
    
    int ret = mysql_query(m_conn, sql.c_str());
    if (ret != 0) {
        m_isValid.store(false);
        return false;
    }
    
    return true;
}

bool MysqlConn::executeCreateTable(const std::string& sql) {
    return execute(sql);
}

bool MysqlConn::executeInsert(const std::string& sql) {
    return execute(sql);
}

bool MysqlConn::executeUpdate(const std::string& sql) {
    return execute(sql);
}

bool MysqlConn::executeDelete(const std::string& sql) {
    return execute(sql);
}

MYSQL_RES* MysqlConn::executeQuery(const std::string& sql) {
    if (!isValid()) {
        return nullptr;
    }
    
    updateLastUsedTime();
    
    int ret = mysql_query(m_conn, sql.c_str());
    if (ret != 0) {
        m_isValid.store(false);
        return nullptr;
    }
    
    return mysql_store_result(m_conn);
}

bool MysqlConn::beginTransaction() {
    if (!isValid()) {
        return false;
    }
    
    updateLastUsedTime();
    
    int ret = mysql_autocommit(m_conn, 0);
    if (ret != 0) {
        m_isValid.store(false);
        return false;
    }
    
    return true;
}

bool MysqlConn::commitTransaction() {
    if (!isValid()) {
        return false;
    }
    
    int ret = mysql_commit(m_conn);
    mysql_autocommit(m_conn, 1);
    
    if (ret != 0) {
        m_isValid.store(false);
        return false;
    }
    
    return true;
}

bool MysqlConn::rollbackTransaction() {
    if (!isValid()) {
        return false;
    }
    
    int ret = mysql_rollback(m_conn);
    mysql_autocommit(m_conn, 1);
    
    if (ret != 0) {
        m_isValid.store(false);
        return false;
    }
    
    return true;
}

bool MysqlConn::ping() {
    if (!m_conn) {
        m_isValid.store(false);
        return false;
    }
    
    int ret = mysql_ping(m_conn);
    if (ret != 0) {
        m_isValid.store(false);
        return false;
    }
    
    m_isValid.store(true);
    return true;
}

unsigned long long MysqlConn::getAffectedRows() const {
    return m_conn ? mysql_affected_rows(m_conn) : 0;
}

unsigned long long MysqlConn::getLastInsertId() const {
    return m_conn ? mysql_insert_id(m_conn) : 0;
}

void MysqlConn::updateLastUsedTime() {
    m_lastUsedTime = std::chrono::steady_clock::now();
}

std::chrono::steady_clock::time_point MysqlConn::getLastUsedTime() const {
    return m_lastUsedTime;
}

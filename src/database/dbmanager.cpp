#include "database/dbmanager.hpp"

dbmanager &dbmanager::instance() {
  static dbmanager instance;
  return instance;
}

dbmanager::dbmanager() { connect(); }
dbmanager::~dbmanager() { conn_.reset(); }

void dbmanager::connect() {
  const std::string conn_string = "host=localhost port=5432 dbname=rstk user=yigit password=password";
  conn_ = std::make_unique<pqxx::connection>(conn_string);
}

pqxx::result dbmanager::executeQuery(const std::string &query) {
  if (!conn_ || !conn_->is_open()) {
    connect();
  }

  pqxx::work txn(*conn_);
  pqxx::result res = txn.exec(query);
  txn.commit();
  return res;
}

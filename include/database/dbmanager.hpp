#pragma once
#ifndef DBMANAGER_HPP
#define DBMANAGER_HPP

#include <memory>
#include <pqxx/pqxx>
#include <string>

class dbmanager {
public:
  static dbmanager &instance();
  pqxx::result executeQuery(const std::string &query);
  ~dbmanager();

private:
  dbmanager();
  void connect();
  std::unique_ptr<pqxx::connection> conn_;
};

#endif // DBMANAGER_HPP

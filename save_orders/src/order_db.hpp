#pragma once

#include <atomic>
#include <chrono>
#include <fstream>
#include <map>
#include <string>

#include <concurrentqueue.h>
#include <qlua>

#include "log_window.hpp"

struct order_event {
  std::chrono::time_point<std::chrono::steady_clock> time;
  qlua::table::row::orders order;
};

struct order_db {
  order_db(const std::string& filename);

  // Receive order information from periodic orders table check
  // or from OnOrder callback
  void update(qlua::extended_api& q, const qlua::table::row::orders& orders);
  // Update order information by order num
  void update(qlua::extended_api& q, const std::string& class_code, const int& order_num);

  void set_log_window(const std::shared_ptr<log_window> log);

  static void saver_thread(order_db& db);
private:
  std::atomic<bool> running_;
  std::atomic<bool> saver_busy_;
  std::ofstream file_;
  std::shared_ptr<log_window> log_;
  moodycamel::ConcurrentQueue<order_event> queue_;
  std::thread thread_;

  // Convert order information to .tri format
  std::map<std::string, std::string> to_tri(const qlua::table::row::orders& orders);

  inline void log(const std::string& m);
};

#pragma once

#include <concurrentqueue.h>
#include <qlua>

struct log_record {
  std::chrono::system_clock::time_point time;
  std::string message;
};

struct log_window {
  enum class columns {
    time = 1,
    message
  };

  log_window::log_window(const qlua::extended_api& q);

  void invoke_on_main();
  void add(const std::string& message);

private:
  qlua::extended_api q_;
  int table_id_{0};
  moodycamel::ConcurrentQueue<log_record> queue_;

  bool create();
  static void table_updater(log_window& window);
  void append_row(const log_record& rec);
};

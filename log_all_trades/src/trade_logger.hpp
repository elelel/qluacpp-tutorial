#pragma once

#include <atomic>
#include <fstream>
#include <thread>

#include <readerwriterqueue.h>
#include <qlua>

struct log_record {
  std::chrono::time_point<std::chrono::system_clock> time;
  qlua::alltrade data;
};

struct trade_logger {
  static trade_logger& instance();

  std::atomic<bool>& running();
  std::atomic<bool>& flusher_busy();
  moodycamel::ReaderWriterQueue<log_record>& queue();
  std::ofstream& file();
  void update(const qlua::alltrade& data);

  static void periodic_flusher(trade_logger& logger);
  
private:
  trade_logger();
  ~trade_logger();

  std::atomic<bool> running_;
  std::atomic<bool> flusher_busy_;
  std::ofstream file_;
  moodycamel::ReaderWriterQueue<log_record> queue_;
  std::thread thread_;

};

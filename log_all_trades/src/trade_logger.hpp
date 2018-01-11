#pragma once

#include <atomic>
#include <fstream>
#include <thread>

#include <readerwriterqueue.h>
#include <qluacpp/qlua>

enum class record_type {
  NONE,
  CLASSES,
  CLASS_INFO,
  ALL_TRADE
};

struct classes_record {
  std::vector<std::string> codes;
};

struct class_info_record {
  std::string name;
  std::string code;
  unsigned int npars;
  unsigned int nsecs;
  std::vector<std::string> securities;
};

struct all_trade_record {
  std::string name;
  std::string class_code;
  std::string sec_code;
  double price;
  double value;
  double qty;
  ::qlua::table::all_trades::date_time trade_datetime;  
};
  
struct log_record {
  std::chrono::time_point<std::chrono::system_clock> time;
  record_type rec_type;

  all_trade_record all_trade;
  classes_record classes;
  class_info_record class_info;
};

struct trade_logger {
  static trade_logger& instance();
  void terminate();
  
  std::atomic<bool>& running();
  std::atomic<bool>& terminated();
  std::atomic<bool>& flusher_busy();
  moodycamel::ReaderWriterQueue<log_record>& queue();
  std::ofstream& file();
  void update(const log_record& rec);

  static void periodic_flusher(trade_logger& logger);
  
private:
  trade_logger();
  ~trade_logger();

  std::atomic<bool> running_;
  std::atomic<bool> terminated_;
  std::atomic<bool> flusher_busy_;
  std::ofstream file_;
  moodycamel::ReaderWriterQueue<log_record> queue_;
  std::thread thread_;

};

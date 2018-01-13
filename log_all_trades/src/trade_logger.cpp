#include "trade_logger.hpp"

#include <ctime>
#include <iostream>
#include <iomanip>

static std::ofstream& operator<<(std::ofstream& file, const log_record& rec) {
  auto remove_crlf = [] (char c[]) {
    const auto len = strlen(c);
    while ((len > 0) && ((c[len-1] == '\n') || (c[len-1] == '\r')))
      c[len-1] = 0x00;
  };

  using std::chrono::system_clock;
  std::time_t tt(system_clock::to_time_t(rec.time));
  char timec_event[256];
  ctime_s(timec_event, 256, &tt);
  remove_crlf(timec_event);
  tt = system_clock::to_time_t(system_clock::now());
  char timec_written[256];
  ctime_s(timec_written, 256, &tt);
  remove_crlf(timec_written);
  file << "Event at " << timec_event << ", "
       << "written at " << timec_written << ". ";

  switch (rec.rec_type) {
  case record_type::ALL_TRADE:
    {
    auto& d = rec.all_trade.trade_datetime;
    file << "Trade: ["
         << "timestamp "
         << std::setfill('0') << std::setw(4) << d.year << "/"
         << std::setfill('0') << std::setw(2) << d.month << "/"
         << std::setfill('0') << std::setw(2) << d.day << " "
         << std::setfill('0') << std::setw(2) << d.hour << ":"
         << std::setfill('0') << std::setw(2) << d.min << ":"
         << std::setfill('0') << std::setw(2) << d.sec << ", "
         << "name " << rec.all_trade.name << ", "
         << "code " << rec.all_trade.sec_code << ", "
         << "price " << rec.all_trade.price << ", "
         << "value " << rec.all_trade.value << ", "
         << "qty " << rec.all_trade.qty
         << "]\n";
    break;
    }
  case record_type::CLASSES: {
    file << "Classes: [";
    bool need_comma{false};
    for (const auto& name : rec.classes.codes) {
      if (need_comma) { file << ", "; };
      file << name;
      need_comma = true;
    }
    file << "]\n";
    break;
  }
  case record_type::CLASS_INFO:
    file << "Class info: ["
         << "name " << rec.class_info.name << ", "
         << "code " << rec.class_info.code << ", "
         << "number of parameters: " << rec.class_info.npars << ", "
         << "number of securities: " << rec.class_info.nsecs << "]\n";
    break;
  default:
    file << "Unknown record type " << int(rec.rec_type) << "\n";
  }
  return file;
}

trade_logger& trade_logger::instance() {
  static trade_logger t;
  return t;
}

trade_logger::trade_logger() :
  running_(false),
  terminated_(false),
  flusher_busy_(false),
  thread_(std::move(std::thread(periodic_flusher, std::ref(*this)))) {
  file_.open("all_trade_log.txt");
  running_ = true;
}

trade_logger::~trade_logger() {
  terminate();
}

void trade_logger::terminate() {
  //  std::cout << "Terminating" << std::endl;
  running_ = false;
  if (!flusher_busy_) {
    //    std::cout << "Flushing" << std::endl;
    // Flush the queue, in case anything is left unwritten
    log_record rec;
    while (queue_.try_dequeue(rec)) {
      file_ << rec;
    }
    file_.close();
  }
  //  std::cout << "Terminated true" << std::endl;
  if (!terminated_) {
    terminated_ = true;
    thread_.join();
  }
}

moodycamel::ReaderWriterQueue<log_record>& trade_logger::queue() {
  return queue_;
}

std::ofstream& trade_logger::file() {
  return file_;
}

std::atomic<bool>& trade_logger::running() {
  return running_;
}

std::atomic<bool>& trade_logger::terminated() {
  return terminated_;
}

std::atomic<bool>& trade_logger::flusher_busy() {
  return flusher_busy_;
}

void trade_logger::update(const log_record& rec) {
  queue_.enqueue(rec);
}

void trade_logger::periodic_flusher(trade_logger& logger) {
  using namespace std::chrono_literals;
  bool done{false};
  while (!logger.terminated()) {
    if (logger.running()) {
      logger.flusher_busy() = true;
      // Write to file once per minute and flush it
      log_record rec;
      while (logger.queue().try_dequeue(rec)) {
        logger.file() << rec;
      }
      logger.file().flush();
      logger.flusher_busy() = false;
    }
    for (int i = 1; i <= 60; ++i) {
      if (logger.terminated()) break;
      std::this_thread::sleep_for(1s);
    }
  }
}

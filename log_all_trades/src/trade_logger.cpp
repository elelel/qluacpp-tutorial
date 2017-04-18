#include "trade_logger.hpp"

#include <ctime>
#include <iostream>

static std::ofstream& operator<<(std::ofstream& file, const log_record& rec) {
  using std::chrono::system_clock;
  std::time_t tt(system_clock::to_time_t(rec.time));
  char timec[256];
  ctime_s(timec, 256, &tt);
  file << timec;
  tt = system_clock::to_time_t(system_clock::now());
  ctime_s(timec, 256, &tt);
  file << timec;
  file << "\t" << rec.data.sec_code
       << "\t" << rec.data.trade_num
       << "\t" << rec.data.price
       << "\t" << rec.data.value
       << "\t" << rec.data.qty
       << "\n";
  return file;
}

trade_logger& trade_logger::instance() {
  static trade_logger t;
  return t;
}

trade_logger::trade_logger() :
  running_(false),
  flusher_busy_(false),
  thread_(std::move(std::thread(periodic_flusher, std::ref(*this)))) {
  file_.open("all_trade_log.txt");
  file_ << "Event time\tTime written\tSecCode\tTrade num\tPrice\tValue\tQty" << std::endl;
  running_ = true;
}

trade_logger::~trade_logger() {
  running_ = false;
  if (!flusher_busy_) {
    // Flush the queue, in case anything is left unwritten
    log_record rec;
    while (queue_.try_dequeue(rec)) {
      file_ << rec;
    }
    file_.close();
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

std::atomic<bool>& trade_logger::flusher_busy() {
  return flusher_busy_;
}

void trade_logger::update(const qlua::alltrade& data) {
  log_record rec{std::chrono::system_clock::now(), data};
  queue_.enqueue(rec);
}

void trade_logger::periodic_flusher(trade_logger& logger) {
  using namespace std::chrono_literals;
  while (true) {
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
    std::this_thread::sleep_for(60s);
  }
}


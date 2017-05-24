#include "trade_logger.hpp"

#include <ctime>
#include <iostream>

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
  file << timec_event
       << "\t" << timec_written
       << "\t" << rec.sec_code
       << "\t" << rec.price
       << "\t" << rec.value
       << "\t" << rec.qty
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
  file_ << "Event time\tTime written\tSecCode\tPrice\tValue\tQty" << std::endl;
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

void trade_logger::update(const log_record& rec) {
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

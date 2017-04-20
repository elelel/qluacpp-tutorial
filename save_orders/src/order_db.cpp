#include "order_db.hpp"

#include <json.hpp>

using json = nlohmann::json;

order_db::order_db(const std::string& filename) :
  thread_(std::move(std::thread(saver_thread, std::ref(*this)))) {
  file_.open(filename);
  running_ = true; 
}

void order_db::set_log_window(const std::shared_ptr<log_window> log) {
  log_ = log;
}

void order_db::update(qlua::extended_api& q, const qlua::table::row::orders& order) {
  auto t = std::chrono::steady_clock::now();
  queue_.enqueue({t, order});
}

void order_db::update(qlua::extended_api& q, const std::string& class_code, const int& order_num) {
  auto order = q.getOrderByNumber(class_code.c_str(), order_num);
  update(q, order);
}

void order_db::log(const std::string& m) {
  if (log_ != nullptr) {
    log_->add(m);
  }
}

void order_db::saver_thread(order_db& db) {
  using namespace std::chrono_literals;
  while (true) {
    if (db.running_) {
      db.saver_busy_ = true;

      
      db.saver_busy_ = false;
    }
    std::this_thread::sleep_for(1s);
  }
}

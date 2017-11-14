#include "bot.hpp"

#include "state.hpp"
#include "status.hpp"

bot& bot::instance() {
  static bot b;
  return b;
}

void bot::main(const lua::state& l) {
  // Make thread-safe bot initialization
  {
    auto& b = bot::instance();
    std::unique_lock<std::mutex> lock(b.mutex_);
    b.cv_.wait(lock, [&b] () { return !b.cb_active_; });
    b.main_active_ = true;
    b.state_ = std::unique_ptr<state>(new state());
    b.status_ = std::unique_ptr<status>(new status(l));
    b.state_->set_lua_state(l);
    b.state_->init_client_info();
    b.state_->request_bid_ask();
    b.status_->update_title();
  }

  auto& b = bot::instance();  
  while (true) {
    std::unique_lock<std::mutex> lock(b.mutex_);
    b.cv_.wait(lock, [&b] () { return b.main_wakeup && !b.cb_active_; });
    b.main_wakeup = false;
    b.main_active_ = true;
    std::cout << "Activated main" << std::endl;
    if (!b.terminated) {
      std::cout << "Refreshing instruments" << std::endl;
      if (b.refresh_instruments) {
        b.state_->refresh_available_instrs();
        // For trading in Quik Junior emulator, remove otherwise
        b.state_->filter_available_instrs_quik_junior(); 
      }
      std::cout << "Checking if periodic triggered" << std::endl;
      if (b.timer_triggered) {
        b.state_->choose_candidates();
        b.timer_triggered = false;
        std::cout << "Restarting periodic thread" << std::endl;
        b.timer_ = std::move(std::thread([&b] () {
              int counter = b.settings().candidates_refresh_timeout * 1000;
              while ((counter != 0) && (!b.terminated)) {
                std::this_thread::sleep_for(std::chrono::milliseconds{100});
                counter -= 100;
              }
              if (!b.terminated) {
                // Wait till main is inactive and wake it up again
                std::unique_lock<std::mutex> lock(b.mutex_);
                b.cv_.wait(lock, [&b] () { return !b.main_active_ && !b.cb_active_; });
                b.timer_triggered = true;
                b.main_wakeup = true;
                lock.unlock();
                std::cout << "Notifying" << std::endl;
                b.cv_.notify_one();
              }
            }));
        std::cout << "Calling detach" << std::endl;
        try {
          b.timer_.detach();
        } catch (std::exception e) {
          auto q = qlua::api(l);
          b.terminate(q, "Failed to detach timer thread, exception: " + std::string(e.what()));
        }
      }
      b.main_active_ = false;
      std::cout << "Unlocking" << std::endl;
      lock.unlock();
      b.cv_.notify_one();
    } else {
      b.main_active_ = false;
      std::cout << "Unlocking (termination)" << std::endl;
      lock.unlock();
      std::cout << "Notifying (termination)" << std::endl;
      b.cv_.notify_one();
      break;
    }
  }
  std::cout << "main callback end" << std::endl;
}

std::tuple<int> bot::on_stop(const lua::state& l,
                             ::lua::entity<::lua::type_policy<int>> signal) {
  auto& b = bot::instance();
  std::unique_lock<std::mutex> lock(b.mutex_);
  b.cv_.wait(lock, [&b] () { return !b.main_active_; });
  b.cb_active_ = true;
  
  b.state_->set_lua_state(l);
  b.state_->on_stop();
  b.terminated = true;

  b.cb_active_ = false;
  lock.unlock();
  b.cv_.notify_one();
  return std::make_tuple(int(1));
}

void bot::on_connected(const lua::state& l,
                       ::lua::entity<::lua::type_policy<bool>> new_class_received) {
  auto& b = bot::instance();
  std::unique_lock<std::mutex> lock(b.mutex_);
  b.cv_.wait(lock, [&b] () { return !b.main_active_; });
  b.cb_active_ = true;
  
  if (new_class_received()) {
    b.refresh_instruments = true;
  }
  b.timer_triggered = true;
  
  b.cb_active_ = false;
  lock.unlock();
  b.cv_.notify_one();
}

void bot::on_order(const lua::state& l,
                   ::lua::entity<::lua::type_policy<::qlua::table::orders>> order) {
  std::cout << "OnOrder" << std::endl;
  auto& b = bot::instance();
  std::unique_lock<std::mutex> lock(b.mutex_);
  b.cv_.wait(lock, [&b] () { return !b.main_active_; });
  b.cb_active_ = true;
  
  b.state_->set_lua_state(l);
  b.state_->on_order(order().trans_id(),
                     order().order_num(),
                     order().flags(),
                     order().qty(),
                     order().balance(),
                     order().price());

  if (b.update_status) {
    std::cout << "Calling status update" << std::endl;
    b.status_->update(l);
    std::cout << "Status update done" << std::endl;
  }
  
  b.cb_active_ = false;
  lock.unlock();
  b.cv_.notify_one();
  std::cout << "OnOrder done" << std::endl;
}

void bot::on_quote(const lua::state& l,
                   ::lua::entity<::lua::type_policy<const char*>> sec_class,
                   ::lua::entity<::lua::type_policy<const char*>> sec_code) {
  std::cout << "OnQuote" << std::endl;
  auto& b = bot::instance();
  std::unique_lock<std::mutex> lock(b.mutex_);
  b.cv_.wait(lock, [&b] () { return !b.main_active_; });
  std::cout << "Waited till main not active" << std::endl;
  b.cb_active_ = true;

  b.state_->set_lua_state(l);
  b.state_->on_quote(std::string(sec_class()), std::string(sec_code()));

  if (b.update_status) {
    std::cout << "Calling status update" << std::endl;
    b.status_->update(l);
    std::cout << "Status update done" << std::endl;
  }

  b.cb_active_ = false;
  lock.unlock();
  b.cv_.notify_one();
  std::cout << "OnQuote done" << std::endl;
}

void bot::terminate(const qlua::api& q, const std::string& msg) {
  if (msg.size() > 0) {
    q.message(msg.c_str());
  }
  auto& b = bot::instance();
  b.instance().terminated = true;
}

settings_record& bot::settings() {
  return settings_;
}

const std::unique_ptr<state>& bot::get_state() {
  return state_;
}

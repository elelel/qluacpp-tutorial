#include "bot.hpp"

#include "state.hpp"
#include "status.hpp"

bot& bot::instance() {
  static bot b;
  return b;
}

void bot::main(const lua::state& l) {
  std::cout << "main entry" << std::endl;
  auto& b = bot::instance();
  // Reinitialize flags
  b.terminated = false;
  b.update_status = true;
  b.refresh_instruments = true;
  b.select_candidates = true;
  b.main_stopped = false;

  std::cout << "setting lua states" << std::endl;
  b.state_ = std::unique_ptr<state>(new state());
  b.state_->set_lua_state(l);
  b.status_ = std::unique_ptr<status>(new status());
  b.status_->set_lua_state(l);
  std::cout << "init client info" << std::endl;
  b.state_->init_client_info();
  std::cout << "request bid ask" << std::endl;
  b.state_->request_bid_ask();
  std::cout << "update title" << std::endl;
  b.status_->update_title();
  std::cout << "starting timer" << std::endl;
  // Start timer loop
  b.timer_ = std::move(std::thread([&b] () {
        for (size_t counter = 0; !b.terminated; counter += 100) {
          if (counter >= b.settings().candidates_refresh_timeout * 1000) {
            b.refresh_instruments = true;
            counter = 0;
          }
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
      }));
  try {
    b.timer_.detach();
  } catch (std::exception e) {
    auto q = qlua::api(l);
    b.terminate(q, "Failed to detach timer thread, exception: " + std::string(e.what()));
  }

  int k{0};
  while (true) {
    std::unique_lock<std::mutex> lock(b.mutex_);
    b.cv_.wait(lock, [&b] () {
        return b.refresh_instruments ||
          b.select_candidates ||
          b.update_status ||
          b.terminated; 
      });
    std::cout << "Woke up main"  << std::endl;

    // Workaround Quik bug to make sure callback handler has finished
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    qlua::extended q(l);
    std::cout << "Calling thread safe" << std::endl;
    try {
      q.thread_safe_exec(l, "l2q_spread_bot_sync_object", "thread_safe_main");
    } catch (std::exception e) {
      b.terminate(q, "Failed to call thread safely, exception: " + std::string(e.what()));
    }
    if (b.terminated) {
      std::cout << "Main terminating" << std::endl;
      b.timer_ = std::move(std::thread());
      b.state_ = nullptr;
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  b.main_stopped = true;
  b.stop_cv_.notify_one();
}

void bot::low_priority_actions(const lua::state& l) {
  auto &b = bot::instance();
  b.state_->set_lua_state(l);
  if (b.refresh_instruments) {
    std::cout << "Refreshing instruments" << std::endl;
    b.state_->refresh_available_instrs();
    // For trading in Quik Junior emulator, remove otherwise
    b.state_->filter_available_instrs_quik_junior();
    b.refresh_instruments = false;
  }

  if (b.select_candidates) {
    std::cout << "Selecting candidates" << std::endl;
    b.state_->choose_candidates();
    b.select_candidates = false;
  }
  
  if (b.update_status) {
    std::cout << "Updating status" << std::endl;
    b.status_->set_lua_state(l);
    b.status_->update();
    b.update_status = false;
  }
}

std::tuple<int> bot::on_stop(const lua::state& l,
                             ::lua::entity<::lua::type_policy<int>> signal) {
  std::cout << "on stop " << std::endl;
  auto& b = bot::instance();
  b.state_->set_lua_state(l);
  b.state_->on_stop();
  b.status_->set_lua_state(l);
  b.status_ = nullptr;
  b.terminated = true;
  std::unique_lock<std::mutex> lock(b.stop_mutex_);
  std::cout << "Waiting for main to stop" << std::endl;
  b.stop_cv_.wait(lock, [&b] () { return b.main_stopped == true; });
  std::cout << "Main stopped" << std::endl;
  lock.unlock();
  return std::make_tuple(int(1));
}

void bot::on_connected(const lua::state& l,
                       ::lua::entity<::lua::type_policy<bool>> new_class_received) {
  std::cout << "on connected " << std::endl;
  auto& b = bot::instance();
  if (new_class_received()) {
    b.refresh_instruments = true;
    b.select_candidates = true;
  }
}

void bot::on_order(const lua::state& l,
                   ::lua::entity<::lua::type_policy<::qlua::table::orders>> order) {
  std::cout << "OnOrder" << std::endl;
  auto& b = bot::instance();
  std::lock_guard<std::mutex> lock(b.mutex_);
  b.state_->set_lua_state(l);
  b.state_->on_order(order().trans_id(),
                     order().order_num(),
                     order().flags(),
                     order().qty(),
                     order().balance(),
                     order().price());
  std::cout << "OnOrder setting update status to true" << std::endl;
  b.update_status = true;
  std::cout << "OnOrder done" << std::endl;
}

void bot::on_quote(const lua::state& l,
                   ::lua::entity<::lua::type_policy<const char*>> sec_class,
                   ::lua::entity<::lua::type_policy<const char*>> sec_code) {
  std::cout << "OnQuote" << std::endl;
  auto& b = bot::instance();
  std::lock_guard<std::mutex> lock(b.mutex_);
  b.state_->set_lua_state(l);
  b.state_->on_quote(std::string(sec_class()), std::string(sec_code()));
  std::cout << "OnQuote requesting update status" << std::endl;
  b.update_status = true;
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

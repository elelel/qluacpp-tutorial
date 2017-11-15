#include "bot.hpp"

#include "state.hpp"
#include "status.hpp"

bot& bot::instance() {
  static bot b;
  return b;
}

void bot::on_init(const lua::state& l, ::lua::entity<::lua::type_policy<const char*>>) {
}

void bot::main(const lua::state& l) {
  auto& b = bot::instance();

  b.main_active_ = true;
  b.state_ = std::unique_ptr<state>(new state());
  b.state_->set_lua_state(l);
  b.status_ = std::unique_ptr<status>(new status(l));
  b.state_->init_client_info();
  b.state_->request_bid_ask();
  b.status_->update_title();
  // Start timer loop
  b.timer_ = std::move(std::thread([&b] () {
        for (size_t counter = 0; !b.terminated; counter += 100) {
          if (counter >= b.settings().candidates_refresh_timeout * 1000) {
            b.refresh_instruments = true;
            counter = 0;
            b.cv_.notify_one();
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
    std::cout << "Main locking" << std::endl;
    //    std::unique_lock<std::mutex> lock(b.mutex_);
    std::cout << "Main waiting " <<
      b.refresh_instruments << " " <<
      b.select_candidates << " " <<
      b.update_status << " " <<
      b.terminated << " " << std::endl;
    /*    b.cv_.wait(lock, [&b] () {
        return b.refresh_instruments ||
          b.select_candidates ||
          b.update_status ||
          b.terminated; 
          });*/
    std::cout << "Woke up main"  << std::endl;

    qlua::extended q(l);
    std::cout << "Calling thread safe" << std::endl;
    try {
      q.quik_thread_safe(l, "thread_safe");
    } catch (std::exception e) {
      b.terminate(q, "Failed to call thread safely, exception: " + std::string(e.what()));
    }
    if (b.terminated) {
      std::cout << "Main terminating" << std::endl;
      break;
    }
    b.refresh_instruments = false;
    b.select_candidates = false;
    b.update_status = false;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }
}

void bot::thread_unsafe(const lua::state& l) {
  auto &b = bot::instance();
  b.state_->set_lua_state(l);
  if (b.refresh_instruments) {
    std::cout << "Refreshing instruments" << std::endl;
    b.state_->refresh_available_instrs();
    // For trading in Quik Junior emulator, remove otherwise
    b.state_->filter_available_instrs_quik_junior();
  }

  if (b.select_candidates) {
    std::cout << "Selecting candidates" << std::endl;
    b.state_->choose_candidates();
  }
  
  if (b.update_status) {
    std::cout << "Updating status" << std::endl;
    b.status_->update(l);
  }

}

std::tuple<bool> bot::thread_safe(const lua::state& l,
                                  ::lua::entity<::lua::type_policy<const int>>,
                                  ::lua::entity<::lua::type_policy<const int>>) {
  std::cout << "Thread safe" << std::endl;
  thread_unsafe(l);
  std::cout << "Thread safe end" << std::endl;
  return std::make_tuple(bool{true});
}

std::tuple<int> bot::on_stop(const lua::state& l,
                             ::lua::entity<::lua::type_policy<int>> signal) {
  std::cout << "on stop " << std::endl;
  auto& b = bot::instance();
  std::cout << "on stop 2 " << std::endl;
  b.state_->set_lua_state(l);
  std::cout << "on stop 3 " << std::endl;
  b.state_->on_stop();
  std::cout << "on stop 4 " << std::endl;
  b.terminated = true;
  std::cout << "on stop 5 " << std::endl;
  b.cv_.notify_one();
  std::cout << "on stop 6 " << std::endl;
  return std::make_tuple(int(1));
}

void bot::on_connected(const lua::state& l,
                       ::lua::entity<::lua::type_policy<bool>> new_class_received) {
  std::cout << "on connected " << std::endl;
  auto& b = bot::instance();
  if (new_class_received()) {
    b.refresh_instruments = true;
    b.select_candidates = true;
    b.cv_.notify_one();
  }
}

void bot::on_order(const lua::state& l,
                   ::lua::entity<::lua::type_policy<::qlua::table::orders>> order) {
  std::cout << "OnOrder" << std::endl;
  auto& b = bot::instance();
  b.state_->set_lua_state(l);
  b.state_->on_order(order().trans_id(),
                     order().order_num(),
                     order().flags(),
                     order().qty(),
                     order().balance(),
                     order().price());
  b.update_status = true;
  b.cv_.notify_one();
  std::cout << "OnOrder done" << std::endl;
}

void bot::on_quote(const lua::state& l,
                   ::lua::entity<::lua::type_policy<const char*>> sec_class,
                   ::lua::entity<::lua::type_policy<const char*>> sec_code) {
  std::cout << "OnQuote" << std::endl;
  auto& b = bot::instance();
  b.state_->set_lua_state(l);
  b.state_->on_quote(std::string(sec_class()), std::string(sec_code()));
  b.update_status = true;
  b.cv_.notify_one();
  std::cout << "OnQuote done" << std::endl;
}

void bot::terminate(const qlua::api& q, const std::string& msg) {
  std::cout << "terminate called " << std::endl;
  if (msg.size() > 0) {
    q.message(msg.c_str());
  }
  auto& b = bot::instance();
  b.instance().terminated = true;
  b.cv_.notify_one();
}

settings_record& bot::settings() {
  return settings_;
}

const std::unique_ptr<state>& bot::get_state() {
  return state_;
}

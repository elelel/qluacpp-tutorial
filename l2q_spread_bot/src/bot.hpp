#pragma once

#include <atomic>
#include <mutex>
#include <condition_variable>

#include "settings.hpp"
#include "state.hpp"
#include "status.hpp"

struct bot {
  static bot& instance(); 
  void set_main_lua_state(const lua::state& main_lua_state);

  // Callbacks for Qlua
  static void bot::main(const lua::state& l);
  static std::tuple<int> on_stop(const lua::state& l,
                                 ::lua::entity<::lua::type_policy<int>> signal);
  static void on_connected(const lua::state& l,
                           ::lua::entity<::lua::type_policy<bool>> new_class_received);
  static void on_order(const lua::state& l,
                       ::lua::entity<::lua::type_policy<::qlua::table::orders>> order);
  static void on_quote(const lua::state& l,
                       ::lua::entity<::lua::type_policy<const char*>> sec_class,
                       ::lua::entity<::lua::type_policy<const char*>> sec_code);

  // Terminate with message
  static void terminate(const qlua::api& q,
                        const std::string& msg = "");

  // Accessors
  settings_record& settings();
  const std::unique_ptr<state>& get_state();

  // Signalling flags
  std::atomic<bool> terminated{false}; // The bot is not running, stop main()
  std::atomic<bool> main_wakeup{true}; // Wakeup main() function
  std::atomic<bool> update_status{true}; // Refresh status table in next callback
  std::atomic<bool> timer_triggered{true}; // It's time to refresh candidates
  std::atomic<bool> refresh_instruments{true}; // Refresh classes/securities
private:
  std::mutex mutex_;

  bool main_active_{false};
  bool cb_active_{false};
  std::condition_variable cv_;

  std::unique_ptr<qlua::api> qlua_main_;
  std::unique_ptr<state> state_;
  std::unique_ptr<status> status_;




  settings_record settings_;

  std::thread timer_;
  
  bot() {};
  
};

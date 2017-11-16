#pragma once

#include <atomic>
#include <mutex>
#include <condition_variable>

#include <qluacpp/api>

#include "settings.hpp"
#include "state.hpp"
#include "status.hpp"

struct bot {
  static bot& instance(); 

  // Callbacks for Qlua
  static void on_init(const lua::state& l, ::lua::entity<::lua::type_policy<const char*>>);
  static void main(const lua::state& l);
  static std::tuple<int> on_stop(const lua::state& l,
                                 ::lua::entity<::lua::type_policy<int>> signal);
  static void on_connected(const lua::state& l,
                           ::lua::entity<::lua::type_policy<bool>> new_class_received);
  static void on_order(const lua::state& l,
                       ::lua::entity<::lua::type_policy<::qlua::table::orders>> order);
  static void on_quote(const lua::state& l,
                       ::lua::entity<::lua::type_policy<const char*>> sec_class,
                       ::lua::entity<::lua::type_policy<const char*>> sec_code);

  // Thread unsafe things to call on main thread
  static void thread_unsafe_main(const lua::state& l);
  // Thread safe wrapper for thread_unsafe
  static std::tuple<bool> thread_safe_main(const lua::state& l,
                                           ::lua::entity<::lua::type_policy<const int>>,
                                           ::lua::entity<::lua::type_policy<const int>>);
  // Terminate with message
  static void terminate(const qlua::api& q,
                        const std::string& msg = "");

  // Accessors
  settings_record& settings();
  const std::unique_ptr<state>& get_state();

  // Signalling flags
  std::atomic<bool> terminated{false}; // The bot is not running, stop main()
  std::atomic<bool> update_status{true}; // Refresh status table in next callback
  std::atomic<bool> refresh_instruments{true}; // Refresh classes/securities
  std::atomic<bool> select_candidates{true}; // Select instruments with good spread from global trade table
  std::atomic<bool> main_stopped{false};
private:
  settings_record settings_;
  std::unique_ptr<state> state_;
  std::unique_ptr<status> status_;

  std::mutex mutex_;
  std::mutex stop_mutex_;

  std::condition_variable cv_;
  std::condition_variable stop_cv_;

  std::thread timer_;
  
  bot() {};
  
};

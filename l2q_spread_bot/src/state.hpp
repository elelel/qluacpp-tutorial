#pragma once

#include <atomic>
#include <chrono>
#include <deque>
#include <map>
#include <mutex>
#include <set>

#include <qluacpp/extended>

#include "status.hpp"

struct bot;

struct state {
  // Type for unique instrument descriptor
  using instrument = std::pair<std::string, std::string>;

  struct order_info {
    unsigned int new_trans_id{0};
    std::string order_key{};
    unsigned int cancel_trans_id{0};
    unsigned int qty;
    double estimated_price; // Estimated optimal price
    double placed_price; // Price for acive order
  };
  
  struct instrument_info {
    double sec_price_step{0.0};
    // How many lots on balance are operated by this bot    
    size_t balance{0};
    double balance_price{0.0};
    // Bot's buy order
    order_info buy_order;
    // Bot's sell order
    order_info sell_order;
    // Are level2 quotes subscribed to?
    bool l2q_subscribed{false};
    // Last known spread
    double spread{0.0};
  };

  state();

  // Set lua::state and qlua::extended private members
  void set_lua_state(const lua::state& l);
  // Update all class name/security name pairs for all classes
  void refresh_available_instrs();
  // Update all class name/security name pairs for a class
  void refresh_available_instrs(const std::string& class_name);
  // Quik Junior does not really update info for most instruments.
  // Filter available instrs to contain only those known to be updated.
  void filter_available_instrs_quik_junior();
  // Initialize client code and account
  void init_client_info();
  // Request best bid/ask parameters for all_instrs_
  void request_bid_ask();
  // Workaround for Quik reporting NUMBER instead of STRING bug
  static double get_param_double_with_workaround(qlua::table::current_trades_getParamEx& param);
  // Select candidates with highest spread and trade volume > 0
  void choose_candidates();
  // Remove instruments that are not active
  void remove_inactive_instruments();
  // Send request to make a new buy order
  void request_new_order(const instrument& instr, const instrument_info& info, order_info& order, const std::string& operation, const size_t qty);
  // Send request to kill existing order
  void request_kill_order(const instrument& instr, order_info& order);
  // Remove unneeded subscription, or make a new one if needed
  void update_l2q_subscription(const instrument& instr, instrument_info& info);
  // Is it ok to make a new transaction within time period
  bool trans_times_limits_ok();
  
  // Consider taking action: make transaction, remove transaction, update subscriptions, etc.
  void act();

  // Handle on_order
  void on_order(unsigned int trans_id, const std::string& order_key, unsigned int flags, const size_t qty, const size_t balance, const double price);
  // Handle level 2 quotes change
  void on_quote(const std::string& class_code, const std::string& sec_code);
  // Handle on_stop event to deinitialize
  void on_stop();

  // Get new unique transaction id
  unsigned int next_trans_id();

  std::string client_code; // Client code
  std::map<std::string, std::string> class_to_accid; // Which account to use for trading a particular class
  std::set<instrument> all_instrs;  // All available instruments
  std::map<instrument, instrument_info> instrs; // Active instruments
  std::deque<std::chrono::time_point<std::chrono::steady_clock>> trans_times_within_hour; // Time of each transaction within last hour
  

private:
  lua::state l_;
  std::unique_ptr<qlua::extended> q_;
  bot& b_;

  unsigned int next_trans_id_{0};
};


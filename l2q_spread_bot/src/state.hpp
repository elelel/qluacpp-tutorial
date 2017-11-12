#pragma once

#include <atomic>
#include <chrono>
#include <deque>
#include <map>
#include <mutex>
#include <set>

#include <qluacpp/qlua>

#include "status.hpp"

struct bot_status;

struct bot_state {
  // Type for unique instrument descriptor
  using instrument = std::pair<std::string, std::string>;

  struct order_info {
    unsigned int new_trans_id{0};
    unsigned int order_key{0};
    unsigned int cancel_trans_id{0};
    unsigned int qty;
    double estimated_price; // Estimated optimal price
    double placed_price; // Price for acive order
  };
  
  struct instrument_info {
    double sec_price_step{0.0};
    // How many lots on balance are operated by this bot    
    size_t bot_balance{0};
    // Bot's buy order
    order_info buy_order;
    // Bot's sell order
    order_info sell_order;
    // Are level2 quotes subscribed to?
    bool l2q_subscribed{false};
    // Last known spread
    double spread{0.0};
  };

  static bot_state& instance();

  // Set lua::state and qlua::api private members
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
  void on_order(unsigned int trans_id, unsigned int order_key, unsigned int flags, const size_t qty, const size_t balance, const double price);
  // Handle level 2 quotes change
  void on_quote(const std::string& class_code, const std::string& sec_code);
  // Handle on_stop event to deinitialize
  void on_stop();

  // Get new unique transaction id
  unsigned int next_trans_id();
  // Terminate bot with message
  void terminate(const std::string& msg = "");

  std::atomic<bool> terminated{false}; // The bot is not running
  std::string client_code; // Client code
  std::map<std::string, std::string> class_to_accid; // Which account to use for trading a particular class
  std::set<instrument> all_instrs;  // All available instruments
  std::map<instrument, instrument_info> instrs; // Active instruments
  std::deque<std::chrono::time_point<std::chrono::steady_clock>> trans_times_within_hour; // Time of each transaction within last hour
  
  // --- SETTINGS ---
  // Order size in lots size for single transaction
  size_t my_order_size{3};
  // What volume to ignore cumulatively when calculating spread in level 2 bid/ask quotes; in multiples of my_lot_size
  double vol_ignore_coeff{1.0};
  // Consider candidates only if the volume is greater than this number
  double min_volume{1.0};
  // Consider candidates only if spread ratio (1 - ask/bid) is greater than this number
  double min_spread{0.001};
  // Max number of instruments to consider as new candidates 
  size_t num_candidates{10};
  // New order speed limit
  size_t max_new_orders_per_hour{200};
  // ----------------

private:
  lua::state l_;
  std::unique_ptr<qlua::api> q_;
  std::unique_ptr<bot_status> status_;

  bot_state() {};

  unsigned int next_trans_id_{0};

};

extern bot_state& state;

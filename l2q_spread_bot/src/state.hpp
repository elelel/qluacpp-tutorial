#pragma once

#include <chrono>
#include <deque>
#include <map>
#include <mutex>
#include <set>

#include <qluacpp/qlua>

struct bot_state {
  // Type for unique instrument descriptor
  using instrument = std::pair<std::string, std::string>;

  struct order_info {
    unsigned int new_trans_id{0};
    unsigned int order_key{0};
    unsigned int cancel_trans_id{0};
    unsigned int qty;
    double price;
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
  // Request best bid/ask parameters for all_instrs_
  void request_bid_ask() const;
  // Select candidates with highest spread and trade volume > 0
  void choose_candidates();
  // Remove instruments that are not active
  void remove_inactive_instruments();
  // Send request to kill existing order
  void request_kill_order(const instrument& instr, instrument_info& info, const bool is_buy);
  // Remove unneeded subscription, or make a new one if needed
  void update_l2q_subscription(const instrument& instr, instrument_info& info);
  
  // Consider taking action: make transaction, remove transaction, update subscriptions, etc.
  void act();

  // Handle on_order
  void on_order(unsigned int trans_id, unsigned int order_key, unsigned int flags, const size_t lots, const double price);
  // Handle level 2 quotes change
  void on_quote(const std::string& class_code, const std::string& sec_code);

  // Close the bot. Remove any pending transactions
  void close();

  // Get new unique transaction id
  unsigned int next_trans_id();

  std::set<instrument> all_instrs;  // All available instruments
  std::map<instrument, instrument_info> instrs; // Active instruments
  std::deque<std::chrono::time_point<std::chrono::system_clock>> last_hour_trans_time; // Time of each transaction within last hour

  // --- SETTINGS ---
  // Order size in lots size for single transaction
  size_t my_order_size{3};
  // What volume to ignore cumulatively when calculating spread in level 2 bid/ask quotes; in multiples of my_lot_size
  double vol_ignore_coeff{1.0};
  // Consider candidates only if the volume is greater than this number
  double min_volume{1.0};
  // Consider candidates only if spread ratio (1 - ask/bid) is greater than this number
  double min_spread{0.0002};
  // Max number of instruments to consider as new candidates 
  size_t num_candidates{10};
  // New order speed limit
  size_t max_new_orders_per_hour{50};
  // ----------------

private:
  lua::state l_;
  std::unique_ptr<qlua::api> q_;


  bot_state() {};

  unsigned int next_trans_id_{0};

};

extern bot_state& state;

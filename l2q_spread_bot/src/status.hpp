#pragma once

#include <qluacpp/qlua>

#include <memory>
#include <vector>

#include "state.hpp"

struct bot_status {
  enum column {
    CLASS = 1,
    NAME,
    BALANCE,
    SPREAD,
    BUY_ORDER,
    SELL_ORDER
  };
  
  static bot_status& instance();

  // Set lua::state and qlua::api private members
  void set_lua_state(const lua::state& l);
  // Create window
  void create_window();
  // Refresh status window
  void update();

  // Close status window
  void close();
private:
  lua::state l_;
  std::unique_ptr<qlua::api> q_;
  int table_id_{0};
  bool window_created_{false};

  bot_status() {};
};

extern bot_status& status;

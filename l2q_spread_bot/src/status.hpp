#pragma once

#include <memory>
#include <mutex>
#include <vector>

#include <qluacpp/extended>

#include "state.hpp"

struct bot;

struct status {
  enum column {
    ACCID = 1,
    CLASS,
    NAME,
    SEC_PRICE_STEP,
    BALANCE,
    SPREAD,
    EST_BUY_PRICE,
    EST_SELL_PRICE,
    PLACED_BUY_PRICE,
    PLACED_SELL_PRICE,
    BUY_ORDER,
    SELL_ORDER
  };

  status(const lua::state& l);
  ~status();

  void set_lua_state(const lua::state& l);

  // Create window
  void create_window();
  // Refresh status window, need lua::state to be passed from callback context
  void update(const lua::state& l);
  // Refresh status window's title
  void update_title();

private:
  lua::state l_;
  qlua::extended q_;
  int table_id_{0};
  bot& b_;
};


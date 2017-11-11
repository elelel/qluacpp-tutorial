#pragma once

#include <qluacpp/qlua>

#include <memory>
#include <vector>

#include "state.hpp"

struct bot_status {
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

  bot_status(const lua::state& l);

  // Create window
  void create_window();
  // Refresh status window
  void update();
  // Refresh status window's title
  void update_title();

  // Close status window
  void close();
private:
  lua::state l_;
  qlua::api q_;
  static int table_id_;
};

extern bot_status& status;

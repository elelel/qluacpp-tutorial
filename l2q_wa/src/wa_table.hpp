#pragma once

#include <qluacpp/qlua>

struct wa_table {
  // Creates QLUA table
  void create(const qlua::api& q, const char* name);
  // Updates table with the data and timestamps it
  void update(const qlua::api& q,
              const double bid_wap, const double bid_quant,
              const double ask_wap, const double ask_quant);

private:
  int table_id_{0};

  // Table column ids
  const int col_desc_{1};
  const int col_value_{2};

  // Table row ids
  int row_time_{-1};
  int row_bid_wap_{-1};
  int row_bid_quant_{-1};
  int row_ask_wap_{-1};
  int row_ask_quant_{-1};
};

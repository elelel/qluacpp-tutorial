#pragma once

#include <qlua>

#include "calendar.hpp"

struct dividends_table {
  // Column ids, QLUA uses ints, we're going to use enum for convinience
  enum class columns {
    ticker = 1,
    date,
    days_left,
    div_return
  };

  dividends_table(const calendar& c);

  void dividends_table::show(qlua::extended_api& q);

private:
  const calendar& calendar_;
  
  double min_div_ret_{0};
  double max_div_ret_{0};
  double div_ret_range_{0};
  int min_days_left_{0};
  int max_days_left_{0};
  int days_left_range_{0};

  int table_id_{0};

  void create_table(qlua::extended_api& q);

  void create_row(qlua::extended_api& q, const calendar_record& rec);

  unsigned int dividends_table::get_ticker_color(const calendar_record& rec) const;
  unsigned int dividends_table::get_days_left_color(const calendar_record& rec) const;
  unsigned int dividends_table::get_div_ret_color(const calendar_record& rec) const;

  
};

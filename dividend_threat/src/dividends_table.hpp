#pragma once

#include <qluacpp/qlua>

#include "calendar.hpp"

struct dividends_table {
  // Column ids, QLUA uses ints, we're going to use enum for convinience
  enum class columns {
    ticker = 1,
    date,
    days_left,
    div_return
  };

  // Initializes variables that may be used in color intensity calculation
  dividends_table(const calendar& c);

  // Ties everything together: creates the table, shows the window and fills the data
  void dividends_table::show(const qlua::api& q);

private:
  const calendar& calendar_;
  
  double min_div_ret_{0};
  double max_div_ret_{0};
  double div_ret_range_{0};
  int min_days_left_{0};
  int max_days_left_{0};
  int days_left_range_{0};

  int table_id_{0};

  // Allocates QLua table with columns
  void create_table(const qlua::api& q);
  // Creates window with the table and adds rows to it  
  void create_window(const qlua::api& q);
  // Adds row to the table and sets apropriate color
  void create_row(const qlua::api& q, const calendar_record& rec);

  // Color intensity calculations
  unsigned int dividends_table::get_ticker_color(const calendar_record& rec) const;
  unsigned int dividends_table::get_days_left_color(const calendar_record& rec) const;
  unsigned int dividends_table::get_div_ret_color(const calendar_record& rec) const;

  
};

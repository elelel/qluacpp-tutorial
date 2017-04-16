#include "dividends_table.hpp"

#include <algorithm>
#include <cmath>

dividends_table::dividends_table(const calendar& c) :
  calendar_(c) {
  auto compare_div_rets = [] (const calendar_record& a,
                              const calendar_record& b) {
    return a.div_return<double>() < b.div_return<double>();
  };
  auto compare_days_left = [] (const calendar_record& a,
                               const calendar_record& b) {
    return a.days_left() < b.days_left();
  };
  min_div_ret_ = std::min_element(c.records().begin(), c.records().end(), compare_div_rets)->div_return<double>();
  max_div_ret_ = std::max_element(c.records().begin(), c.records().end(), compare_div_rets)->div_return<double>();
  div_ret_range_ = max_div_ret_ - min_div_ret_;
  min_days_left_ = std::min_element(c.records().begin(), c.records().end(), compare_days_left)->days_left();
  max_days_left_ = std::max_element(c.records().begin(), c.records().end(), compare_days_left)->days_left();
  days_left_range_ = max_days_left_ - min_days_left_;
}

void dividends_table::create_table(qlua::extended_api& q) {
  table_id_ = q.AllocTable();
  auto rc = q.AddColumn(table_id_, int(columns::ticker), "Ticker", true, q.constants().QTABLE_STRING_TYPE(), 15);
  if (!rc) {
    q.message("dividend_threat: Could not create table column for ticker");
  }
  rc = q.AddColumn(table_id_, int(columns::date), "Cutoff date", true, q.constants().QTABLE_STRING_TYPE(), 20);
  if (!rc) {
    q.message("dividend_threat: Could not create table column for cutoff date");
  }
  rc = q.AddColumn(table_id_, int(columns::days_left), "Days left", true, q.constants().QTABLE_STRING_TYPE(), 15);
  if (!rc) {
    q.message("dividend_threat: Could not create table column for days left");
  }
  rc = q.AddColumn(table_id_, int(columns::div_return), "Dividend return", true, q.constants().QTABLE_STRING_TYPE(), 20);
  if (!rc) {
    q.message("dividend_threat: Could not create table column for dividend return");
  }
}

unsigned int dividends_table::get_ticker_color(const calendar_record& rec) const {
  double days_left_coef = ((max_days_left_ - rec.days_left()) / days_left_range_) / 2.0;
  double div_ret_coef = ((rec.div_return<double>() - min_div_ret_) / div_ret_range_) / 2.0;
  double combined_coef = days_left_coef + div_ret_coef;
  double intensity = double{255.0} - (double{255.0} * combined_coef);
  return ((int)floor(intensity) << 16) | ((int)floor(intensity) << 8) | 0xff;
}

unsigned int dividends_table::get_days_left_color(const calendar_record& rec) const {
  double days_left_coef = (max_days_left_ - rec.days_left()) / days_left_range_;
  double intensity = (double{255.0} - (double{255.0} * days_left_coef)) / 2.0;
  return ((int)floor(intensity) << 16) | ((int)floor(intensity) << 8) | 0xff;
}

unsigned int dividends_table::get_div_ret_color(const calendar_record& rec) const {
  double div_ret_coef = (rec.div_return<double>() - min_div_ret_) / div_ret_range_;
  double intensity = (double{255.0} - (double{255.0} * div_ret_coef)) / 2.0;
  return ((int)floor(intensity) << 16) | ((int)floor(intensity) << 8) | 0xff;
}

void dividends_table::create_row(qlua::extended_api& q, const calendar_record& rec) {
  int row_num = q.InsertRow(table_id_, -1);
  q.SetCell(table_id_, row_num, int(columns::ticker), rec.ticker().c_str());
  q.SetCell(table_id_, row_num, int(columns::date), rec.date<const std::string&>().c_str());
  q.SetCell(table_id_, row_num, int(columns::days_left), std::to_string(rec.days_left()).c_str());
  q.SetCell(table_id_, row_num, int(columns::div_return), rec.div_return<const std::string&>().c_str());
  auto def_color = q.constants().QTABLE_DEFAULT_COLOR();
  q.SetColor(table_id_, row_num, int(columns::ticker), get_ticker_color(rec), def_color, def_color, def_color);
  q.SetColor(table_id_, row_num, int(columns::date), get_days_left_color(rec), def_color, def_color, def_color);
  q.SetColor(table_id_, row_num, int(columns::days_left), get_days_left_color(rec), def_color, def_color, def_color);
  q.SetColor(table_id_, row_num, int(columns::div_return), get_div_ret_color(rec), def_color, def_color, def_color);
}

void dividends_table::show(qlua::extended_api& q) {
  create_table(q);
  if (table_id_ != 0) {
    if (q.CreateWindow(table_id_)) {
      q.SetWindowCaption(table_id_, "Dividends-induced market price devaluation threat pressure");
    } else {
      q.message("Could not create window for dividend threat table");
    }
    for (const auto& rec : calendar_.records()) {
      create_row(q, rec);
    }
  } else {
    q.message("Could not create dividend threat table");
  }
}

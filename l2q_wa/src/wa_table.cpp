#include "wa_table.hpp"

#include <chrono>
#include <ctime>
#include <sstream>

void wa_table::create(const qlua::api& q, const char* name) {
  // Allocate Qlua table
  table_id_ = q.AllocTable();
  if (table_id_ == 0) {
    q.message("l2q_wa: Could not alloc table");
    return;
  }
  // Get Qlua constant value
  auto qtable_string_type = q.constant<unsigned int>("QTABLE_STRING_TYPE");
  // Create table's columns
  auto rc = q.AddColumn(table_id_, col_desc_, "Desc", true, qtable_string_type, 25);
  rc = rc * q.AddColumn(table_id_, col_value_, "Value", true, qtable_string_type, 35);
  if (!rc) {
    q.message("l2q_wa: Could not create table columns");
    return;
  }
  // Create table's window and set caption
  if (q.CreateWindow(table_id_)) {
    q.SetWindowCaption(table_id_, name);
  } else {
    q.message("l2wa_wa: Could not create window");
  }
  // Create rows
  row_time_ = q.InsertRow(table_id_, -1);
  row_bid_wap_ = q.InsertRow(table_id_, -1);
  row_bid_quant_ = q.InsertRow(table_id_, -1);
  row_ask_wap_ = q.InsertRow(table_id_, -1);
  row_ask_quant_ = q.InsertRow(table_id_, -1);
  // Create labels for each row
  q.SetCell(table_id_, row_time_, col_desc_, "Time");
  q.SetCell(table_id_, row_bid_wap_, col_desc_, "Bid WAP");
  q.SetCell(table_id_, row_bid_quant_, col_desc_, "Bid vol");
  q.SetCell(table_id_, row_ask_wap_, col_desc_, "Ask WAP");
  q.SetCell(table_id_, row_ask_quant_, col_desc_, "Ask vol");
}

void wa_table::update(const qlua::api& q,
                      const double bid_wap, const double bid_quant,
                      const double ask_wap, const double ask_quant) {
  // Create time string for the timestamp
  using std::chrono::system_clock;
  // Lambda to remove unneeded characters from the end of time strings
  auto remove_crlf = [] (char c[]) {
    const auto len = strlen(c);
    while ((len > 0) && ((c[len-1] == '\n') || (c[len-1] == '\r')))
      c[len-1] = 0x00;
  };
  std::time_t tt = system_clock::to_time_t(system_clock::now());
  char timec_event[256];
  ctime_s(timec_event, 256, &tt);
  remove_crlf(timec_event);

  // Simple lambda to format doubles as strings
  auto double2string = [] (const double d) {
    std::stringstream ss;
    ss << d;
    return ss.str();
  };

  // Set table values
  q.SetCell(table_id_, row_time_, col_value_, timec_event);
  q.SetCell(table_id_, row_bid_wap_, col_value_, double2string(bid_wap).c_str());
  q.SetCell(table_id_, row_bid_quant_, col_value_, double2string(bid_quant).c_str());
  q.SetCell(table_id_, row_ask_wap_, col_value_, double2string(ask_wap).c_str());
  q.SetCell(table_id_, row_ask_quant_, col_value_, double2string(ask_quant).c_str());
}

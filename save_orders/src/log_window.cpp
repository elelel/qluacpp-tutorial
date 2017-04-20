#include "log_window.hpp"

#include <ctime>

log_window::log_window(const qlua::extended_api& q) :
  q_(q) {
  if (!create()) {
    q_.message("Save orders: could not create log window!");
  }
}

bool log_window::create() {
  table_id_ = q_.AllocTable();
  if (table_id_ != 0) {
    auto rc = q_.AddColumn(table_id_, int(columns::time), "Time", true, q_.constants().QTABLE_STRING_TYPE(), 15);
    if (!rc) return false;
    rc = q_.AddColumn(table_id_, int(columns::message), "Message", true, q_.constants().QTABLE_STRING_TYPE(), 80);
    if (!rc) return false;
    if (q_.CreateWindow(table_id_)) {
      q_.SetWindowCaption(table_id_, "Orders saver");
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
}

void log_window::add(const std::string& message) {
  log_record r{std::chrono::system_clock::now(), message};
  queue_.enqueue(r);
}

void log_window::append_row(const log_record& rec) {
  auto remove_crlf = [] (char c[]) {
    const auto len = strlen(c);
    while ((len > 0) && ((c[len-1] == '\n') || (c[len-1] == '\r')))
      c[len-1] = 0x00;
  };
  using std::chrono::system_clock;
  std::time_t tt(system_clock::to_time_t(rec.time));
  char timec_event[256];
  ctime_s(timec_event, 256, &tt);
  remove_crlf(timec_event);
  auto row_num = q_.InsertRow(table_id_, -1);
  q_.SetCell(table_id_, row_num, int(columns::time), timec_event);
  q_.SetCell(table_id_, row_num, int(columns::message), rec.message.c_str());
}

void log_window::invoke_on_main() {
  using namespace std::chrono_literals;
  
  log_record rec;
  std::vector<log_record> records;
  while (queue_.try_dequeue(rec)) records.push_back(rec);
  std::sort(records.begin(), records.end(),
            [] (const log_record& a,
                const log_record& b) {
              return a.time < b.time;
            });
  for (const auto& m : records) append_row(m);
}

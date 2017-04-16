#pragma once

#include <ctime>
#include <string>
#include <vector>

struct calendar_record {
  calendar_record(const std::string& ticker, const std::string& date_str, const std::string& div_return);

  // Ticker for the calendar record
  const std::string& ticker() const;
  // Cutoff date as different types
  template <typename T>
  T date() const;
  // Dividend returns as different types
  template <typename T>
  T div_return() const;

  // How many days are left till the cutoff date
  int days_left() const;

private:
  // Returns record date as std::tm 
  std::tm date_tm() const;

  std::string ticker_;
  std::string date_str_;
  std::string div_return_;
};

struct calendar {
  bool download(const std::string& url);
  const std::string& status_log() const;
  const std::vector<calendar_record>& records() const;

  std::string extract_dividends_block(const std::string& html);
  calendar_record extract_record(const std::string& record_block);
  std::vector<calendar_record> extract_records(const std::string& div_block);

private:
  std::vector<calendar_record> records_;
  std::string status_log_;
  
};

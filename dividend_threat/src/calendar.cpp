#include "calendar.hpp"

#include <cmath>
#include <regex>

#include <curl/curl.h>

calendar_record::calendar_record(const std::string& ticker, const std::string& date_str, const std::string& div_return) :
  ticker_(ticker),
  date_str_(date_str),
  div_return_(div_return) {
  
}

const std::string& calendar_record::ticker() const {
  return ticker_;
}

template <>
const std::string& calendar_record::date() const {
  return date_str_;
}

template <>
std::tm calendar_record::date() const {
  const auto& s = date_str_;
  std::string year = s.substr(s.size() - 4);
  std::string month = s.substr(s.size() - 4 - 3, 2);
  std::string day = s.substr(s.size() - 4 - 3 - 3, 2);
  std::tm tm{0};
  tm.tm_year = std::stoi(year) - 1900;
  tm.tm_mon = std::stoi(month) - 1;
  tm.tm_mday = std::stoi(day);
  return tm;
}

template <>
std::time_t calendar_record::date() const {
  auto tm = date<std::tm>();
  return mktime(&tm);
}

template <>
double calendar_record::div_return() const {
  return std::stod(div_return_);
}

template <>
const std::string& calendar_record::div_return() const {
  return div_return_;
}

int calendar_record::days_left() const {
  auto record_tt = date<std::time_t>();
  std::time_t today_tt = std::time(0);
  return int(floor(std::difftime(record_tt, today_tt) / (60.0*60.0*24.0)));
}

static size_t buf_write_callback(void *bufs, size_t buf_sz, size_t n_bufs, void* out) {
  auto out_string = reinterpret_cast<std::string*>(out);
  auto size_bytes = buf_sz * n_bufs;
  for (size_t i = 0; i < size_bytes; ++i)
    out_string->push_back(*((char*)bufs+i));
  return size_bytes;
}

const std::string& calendar::status_log() const {
  return status_log_;
}

const std::vector<calendar_record>& calendar::records() const {
  return records_;
}

bool calendar::download(const std::string& url) {
  bool rslt = false;
  std::string response;

  curl_global_init(CURL_GLOBAL_ALL);
  CURL *curl_handle = curl_easy_init();
  curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, buf_write_callback);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&response);
  curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "qluacpp-tutorial-agent/1.0");
  CURLcode res = curl_easy_perform(curl_handle);
  if (res == CURLE_OK) {
    status_log_ += "Downloaded " + url + ", length " + std::to_string(response.size()) + " bytes\n";
    auto div_block = extract_dividends_block(response);
    if (!div_block.empty()) {
      records_ = extract_records(div_block);
      rslt = true;
    }
  } else {
    status_log_ += "Curl error while downloading " + url + std::string(": ") + curl_easy_strerror(res) + "\n";
  }
  curl_easy_cleanup(curl_handle);
  curl_global_cleanup();
  return rslt;
}

std::string calendar::extract_dividends_block(const std::string& html) {
  std::string rslt;
  try {
    std::string tmp(html);
    auto pos = tmp.find("shares_filter");
    if (pos != std::string::npos) {
      tmp = tmp.substr(pos + 1);
      pos = tmp.find("</div>");
      if (pos != std::string::npos) {
        tmp = tmp.substr(pos + 1);
        pos = tmp.find("</tr>");
        if (pos != std::string::npos) {
          tmp = tmp.substr(pos + 1);
          pos = tmp.find("</table>");
          if (pos != std::string::npos) {
            rslt = tmp.substr(1, pos);
            status_log_ += "Extracted dividends block of length " + std::to_string(rslt.size()) + "\n";
          }
        }
      }
    }
  } catch (std::exception e) {
    status_log_ += std::string("Exception while trying to extract dividends block: ")
      + e.what() + "\n";
  }
  return rslt;
}

calendar_record calendar::extract_record(const std::string& html) {
  std::string rest(html);
  size_t pos{0};
  size_t column{0};
  std::string ticker{""};
  std::string date{""};
  std::string div_ret{""};
  while (pos != std::string::npos) {
    pos = rest.find("<td>");
    if (pos != std::string::npos) {
      rest = rest.substr(pos + 4);
      pos = rest.find("</td>");
      if (pos != std::string::npos) {
        auto val = rest.substr(0, pos);
        rest = rest.substr(pos + 5);

        switch (column) {
        case 1:
          // Ticker
          ticker = val;
          break;
        case 3:
          {
            // Date
            val.erase(std::remove(val.begin(), val.end(), '\n'), val.end());
            std::regex r(R"(.*?(\d\d\.\d\d.\d\d\d\d).*)");
            std::smatch m;
            if (std::regex_match(val, m, r)) {
              if (m.size() >= 2) {
                date = m[1].str();
              }
            }
          }
          break;
        case 7:
        case 8:
          {
            val.erase(std::remove(val.begin(), val.end(), '\n'), val.end());
            std::regex r(R"(.*?(\d+)[,\.](\d+)%.*?)");
            std::smatch m;
            if (std::regex_match(val, m, r)) {
              if (m.size() >= 3) {
                div_ret = m[1].str() + "." + m[2].str();
              }
            }
          }
          break;
        default:
          break;
        }
      }
    }
    ++column;
  }

  if (!ticker.empty() && !date.empty() && !div_ret.empty()) {
    return calendar_record(ticker, date, div_ret);
  }

  throw std::runtime_error("Bad calendar record. Ticker: " + ticker + ", date: " + date + ", div_ret: " + div_ret + " can't convert: " + html);
}

std::vector<calendar_record> calendar::extract_records(const std::string& html) {
  std::vector<calendar_record> rslt;
  size_t pos{0};
  std::string rest(html);
  size_t counter{0};
  while (pos != std::string::npos) {
    pos = rest.find("</tr>");
    if (pos != std::string::npos) {
      auto first = rest.substr(0, pos);
      rest = rest.substr(pos + 1);
      try {
        auto r = extract_record(first);
        rslt.push_back(r);
      } catch (...) {
      }
      ++counter;
    }
  }
  status_log_ += "Examined " + std::to_string(counter) + ", extracted " + std::to_string(rslt.size()) + " records from html\n";
  return rslt;
}


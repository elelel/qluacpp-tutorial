#pragma once

#include <list>
#include <memory>
#include <mutex>
#include <string>

#include <qluacpp/qlua>

struct model {
  struct candle {
    double high;
    double low;
    double open;
    double close;
    double volume;
    double idx;
  };

  struct view_data {
    std::list<candle> candles;
    double max_high{0};
    double min_high{0};
    double max_low{0};
    double min_low{0};
    double max_open{0};
    double min_open{0};
    double max_close{0};
    double min_close{0};
    double max_price{0};
    double min_price{0};
    double max_volume{0};
    double min_volume{0};
  };

  model(const qlua::api& q,
        const std::string& sec_class, const std::string& sec_code, const unsigned int interval,
        const size_t max_count);
  void update(unsigned int idx);
  
  const view_data& view();
  size_t& max_count();
  std::string& sec_class();
  std::string& sec_code();
  unsigned int& interval();
  
private:
  std::string sec_class_;
  std::string sec_code_;
  unsigned int interval_;
  size_t max_count_{25};

  std::unique_ptr<qlua::data_source> ds_;
  unsigned int last_candle_idx_{0};

  view_data view_data_;

  std::mutex mutex_;

};

#pragma once

#include <list>
#include <string>

#include <qluacpp/qlua>
#include <readerwriterqueue.h>

struct model {
  struct candle {
    double high;
    double low;
    double open;
    double close;
    double volume;
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
        const std::string& tag,
        const size_t max_count);
  void update();
  const view_data& view();
  size_t& max_count();
  std::string& tag();
  
private:
  std::string tag_;
  size_t max_count_{25};

  qlua::api q_;
  moodycamel::ReaderWriterQueue<candle> queue_;
  unsigned int last_candle_idx_{0};
  view_data view_data_;
};

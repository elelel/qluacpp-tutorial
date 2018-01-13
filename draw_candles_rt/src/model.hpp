#pragma once


#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <qluacpp/qlua>

struct model {
  struct candle {
    double high;
    double low;
    double open;
    double close;
    double volume;
    double idx;
    std::string time;
  };

  model(const qlua::api& q,
        const std::string& sec_class, const std::string& sec_code, const unsigned int interval,
        const size_t max_count);
  ~model();
  void update(unsigned int idx);
  
  const std::vector<candle>& candles() const;
  double min_price() const;
  double max_price() const;
  double min_volume() const;
  double max_volume() const;
  
  void wait();
  void notify();
  size_t& max_count();
  std::string& sec_class();
  std::string& sec_code();
  unsigned int& interval();


  std::function<void()> on_new_data = [] () { return; };
private:
  std::string sec_class_;
  std::string sec_code_;
  unsigned int interval_;
  size_t max_count_{25};

  std::unique_ptr<qlua::data_source> ds_;
  unsigned int last_candle_idx_{0};

  std::vector<candle> candles_;
  double min_price_;
  double max_price_;
  double min_volume_;
  double max_volume_;

  std::condition_variable cv_;
  bool ready_{true};
  std::mutex mutex_;

  // Fills candle data from candle in datasource by index
  void fill_candle_from_ds(candle& c, const unsigned int idx); 

};

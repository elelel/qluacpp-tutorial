#include "model.hpp"

#include "windows.h"

#include <algorithm>

model::model(const qlua::api& q,
             const std::string& sec_class, const std::string& sec_code, const unsigned int interval,
             const size_t max_count) :
  sec_class_(sec_class),
  sec_code_(sec_code),
  interval_(interval),
  max_count_(max_count) {

  try {
    ds_ = std::unique_ptr<qlua::data_source>
      (new ::qlua::data_source
       (q.CreateDataSource(sec_class_.c_str(), sec_code_.c_str(), interval_, "last")));
    if (!ds_->SetUpdateCallback("qluacpp_candles_cb")) {
      std::string msg = "Failed to set update callback on datasource: false returned";
      q.message(msg.c_str());
    }
  } catch (std::runtime_error e) {
    std::string msg("Exception while creating datasource: ");
    msg += e.what();
    q.message(msg.c_str());
  }
  }

void model::update(const unsigned int idx) {
  const unsigned int ds_sz = ds_->Size();
  if (ds_sz >= max_count_) {
    std::lock_guard<std::mutex> guard(mutex_);
    bool need_reload{false};
    if (last_candle_idx_ == ds_->Size()) {
      // We don't need to receive new candles window, only update existing candle
      auto found = std::find_if(view_data_.candles.begin(), view_data_.candles.end(),
                                [&idx] (const candle& c) {
                                  return c.idx == idx;
                                });
      if (found != view_data_.candles.end()) {
        found->high = ds_->H(idx);
        found->low = ds_->L(idx);
        found->open = ds_->O(idx);
        found->close = ds_->C(idx);
        found->volume = ds_->V(idx);
      } else {
        need_reload = true;
      }
    } else {
      need_reload = true;
    }

    if (need_reload) {
      // We need to receive new candles window
      view_data_.candles.clear();
      for (size_t i = 0; i < max_count_; ++i) {
        auto idx = ds_sz - max_count_ + 1;
        candle c;
        c.high = ds_->H(idx);
        c.low = ds_->L(idx);
        c.open = ds_->O(idx);
        c.close = ds_->C(idx);
        c.volume = ds_->V(idx);
        c.idx = idx;
        view_data_.candles.push_back(c);
        last_candle_idx_ = idx;
      }
      
    }
  }
}

size_t& model::max_count() {
  return max_count_;
}

std::string& model::sec_class() {
  return sec_class_;
}

std::string& model::sec_code() {
  return sec_code_;
}

unsigned int& model::interval() {
  return interval_;
}

auto model::view() -> const view_data& {
  std::lock_guard<std::mutex> guard(mutex_);
  auto& l = view_data_.candles;
  auto& d = view_data_;

  if (l.size() > 0) {
    d.max_high = std::max_element(l.begin(), l.end(), [] (const candle& a, const candle& b) {
        return a.high < b.high;
      })->high;
    d.min_high = std::min_element(l.begin(), l.end(), [] (const candle& a, const candle& b) {
        return a.high < b.high;
      })->high;
    d.max_low = std::max_element(l.begin(), l.end(), [] (const candle& a, const candle& b) {
        return a.low < b.low;
      })->low;
    d.min_low = std::min_element(l.begin(), l.end(), [] (const candle& a, const candle& b) {
        return a.low < b.low;
      })->low;
    d.max_open = std::max_element(l.begin(), l.end(), [] (const candle& a, const candle& b) {
        return a.open < b.open;
      })->open;
    d.min_open = std::min_element(l.begin(), l.end(), [] (const candle& a, const candle& b) {
        return a.open < b.open;
      })->open;
    d.max_close = std::max_element(l.begin(), l.end(), [] (const candle& a, const candle& b) {
        return a.close < b.close;
      })->close;
    d.min_close = std::min_element(l.begin(), l.end(), [] (const candle& a, const candle& b) {
        return a.close < b.close;
      })->close;
    d.max_volume = std::max_element(l.begin(), l.end(), [] (const candle& a, const candle& b) {
        return a.volume < b.volume;
      })->volume;
    d.min_volume = std::min_element(l.begin(), l.end(), [] (const candle& a, const candle& b) {
        return a.volume < b.volume;
      })->volume;

    std::vector<double> prices{d.max_high, d.min_high, d.max_low, d.min_low, d.max_open, d.min_open, d.max_close, d.min_close};
    d.max_price = *std::max_element(prices.begin(), prices.end());
    d.min_price = *std::min_element(prices.begin(), prices.end());
  }
  return view_data_;
}


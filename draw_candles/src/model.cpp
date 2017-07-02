#include "model.hpp"

#include <algorithm>

model::model(const qlua::api& q, const std::string& tag, const size_t max_count) :
  q_(q),
  tag_(tag),
  max_count_(max_count) {
  update();
  }

void model::update() {
  auto total = q_.getNumCandles(tag_.c_str());
  if (total >= max_count_) {
    auto quik_candles_reader = [this]
      (const ::lua::entity<::lua::type_policy<::qlua::table::candle>>& c, // структура со свечкой
       const unsigned int i, // индекс свечки - not used
       const ::lua::entity<::lua::type_policy<unsigned int>>& n, // количество свечек - not used
       const ::lua::entity<::lua::type_policy<const char*>>& l // легенда (подпись) графика - not used
       ) {
      queue_.enqueue({c().high(), c().low(), c().open(), c().close(), c().volume()});
    };
    
    // Read only candles starting after the last candle received,
    // but no more than the last max_count_ candles
    auto start_idx = last_candle_idx_ + 1;
    if (start_idx < (total - max_count_)) start_idx = total - max_count_;
    auto count = total - start_idx;
    q_.getCandlesByIndex(tag_.c_str(), 0, start_idx, count, quik_candles_reader);
    last_candle_idx_ = start_idx + count;
  }
}

size_t& model::max_count() {
  return max_count_;
}

std::string& model::tag() {
  return tag_;
}

// Make window of last max_count_ candles and thier aggregates
auto model::view() -> const view_data& {
  candle c;
  auto& l = view_data_.candles;
  auto& d = view_data_;
  while (queue_.try_dequeue(c)) {
    l.push_back(c);
    if (l.size() > max_count_) l.pop_front();
  };

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


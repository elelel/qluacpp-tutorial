#include "model.hpp"

#include <algorithm>

model::model(const qlua::api& q, const std::string& tag, const size_t max_count) :
  tag_(tag),
  max_count_(max_count) {
  auto total = q.getNumCandles(tag_.c_str());
  if (total >= max_count_) {
    auto quik_candles_reader = [this]
      (const ::lua::entity<::lua::type_policy<::qlua::table::candle>>& t, // структура со свечкой
       const unsigned int i, // индекс свечки - not used
       const ::lua::entity<::lua::type_policy<unsigned int>>& n, // количество свечек - not used
       const ::lua::entity<::lua::type_policy<const char*>>& l // легенда (подпись) графика - not used
       ) {
      update(t);
    };
    
    // Read only candles that we need from Quik
    q.getCandlesByIndex(tag_.c_str(), 0, total - max_count_, max_count_, quik_candles_reader);
  }
  }

void model::update(const ::lua::entity<::lua::type_policy<::qlua::table::candle>>& lua_candle) {
  queue_.enqueue({lua_candle().high(), lua_candle().low(),
        lua_candle().open(), lua_candle().close(), lua_candle().volume()});
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


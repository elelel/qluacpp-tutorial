#include "model.hpp"

#include "windows.h"

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <sstream>

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
       (q.CreateDataSource(sec_class_.c_str(), sec_code_.c_str(), interval_)));
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

model::~model() {
  if (ds_) {
    try {
      ds_->SetEmptyCallback();
    } catch (std::runtime_error e) {
      std::string msg("Failed to set empty callback on model destruction: ");
      msg += e.what();
      MessageBoxA(NULL, msg.c_str(), "Draw candles RT error", MB_OK);
    }
    /*
    // Close() returns nil instead of bool because of Quik bug, reported and will be fixed 
    try {
      ds_->Close();
    } catch (std::runtime_error e) {
      std::string msg("Failed to close datasource on model destruction: ");
      msg += e.what();
      MessageBoxA(NULL, msg.c_str(), "Draw candles RT error", MB_OK);
      }*/
  }
}

void model::fill_candle_from_ds(candle& c, const unsigned int idx) {
  c.idx = idx;
  c.high = ds_->H(idx);
  c.low = ds_->L(idx);
  c.open = ds_->O(idx);
  c.close = ds_->C(idx);
  c.volume = ds_->V(idx);
  try {
    std::stringstream ss;
    auto t = ds_->T(idx);
    ss << std::setfill('0') << std::setw(4) << t.year << "/"
       << std::setfill('0') << std::setw(2) << t.month << "/"
       << std::setfill('0') << std::setw(2) << t.day << " "
       << std::setfill('0') << std::setw(2) << t.hour << ":"
       << std::setfill('0') << std::setw(2) << t.min << ":"
       << std::setfill('0') << std::setw(2) << t.sec << "."
       << std::setfill('0') << std::setw(2) << t.ms;
    c.time = ss.str();
  } catch (...) {
    std::cout << "Failed to store candle time" << std::endl;
  }
}

void model::update(const unsigned int idx) {
  using namespace std::chrono_literals;
  const unsigned int ds_sz = ds_->Size();
  if (ds_sz >= max_count_) {
    wait();
    if ((idx >= ds_sz - max_count_) && (idx <= ds_sz)) {
      // We don't need to receive new candles window, only update existing candle
      auto found = std::find_if(candles_.begin(), candles_.end(),
                                [&idx] (const candle& c) {
                                  return c.idx == idx;
                                });
      if (found != candles_.end()) {
        fill_candle_from_ds(*found, idx);
      } else {
        // We need to receive new candles window
        candles_.clear();
        candles_.reserve(max_count_);
        for (size_t i = ds_sz - max_count_ + 1; i < ds_sz + 1; ++i) {
          candle c;
          fill_candle_from_ds(c, i);
          candles_.push_back(c);
          last_candle_idx_ = i;
        }
      }
      // Recalculate min/max
      min_price_ = std::numeric_limits<double>::max();
      max_price_ = std::numeric_limits<double>::min();
      min_volume_ = std::numeric_limits<double>::max();
      max_volume_ = std::numeric_limits<double>::min();
      for (const auto& c : candles_) {
        if (min_price_ > c.low) min_price_ = c.low;
        if (max_price_ < c.high) max_price_ = c.high;
        if (min_volume_ > c.volume) min_volume_ = c.volume;
        if (max_volume_ < c.volume) max_volume_ = c.volume;
      }
      on_new_data();
    }
    notify();
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

auto model::candles() const -> const std::vector<candle>& {
  return candles_;
}

double model::max_price() const {
  return max_price_;
}

double model::min_price() const {
  return min_price_;
}

double model::max_volume() const {
  return max_volume_;
}

double model::min_volume() const {
  return min_volume_;
}

void model::notify() {
  std::unique_lock<std::mutex> lock(mutex_);
  ready_ = true;
  lock.unlock();
  cv_.notify_one();
}

void model::wait() {
  std::unique_lock<std::mutex> lock(mutex_);
  cv_.wait(lock, [this] () {return ready_;});
  ready_ = false;
}

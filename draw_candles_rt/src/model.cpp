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
        found->high = ds_->H(idx);
        found->low = ds_->L(idx);
        found->open = ds_->O(idx);
        found->close = ds_->C(idx);
        found->volume = ds_->V(idx);
      } else {
        // We need to receive new candles window
        candles_.clear();
        candles_.reserve(max_count_);
        for (size_t i = ds_sz - max_count_ + 1; i < ds_sz + 1; ++i) {
          candle c;
          c.high = ds_->H(i);
          c.low = ds_->L(i);
          c.open = ds_->O(i);
          c.close = ds_->C(i);
          c.volume = ds_->V(i);
          c.idx = i;
          candles_.push_back(c);
          last_candle_idx_ = i;
        }
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

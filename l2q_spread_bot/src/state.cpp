#include "state.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>

#include "status.hpp"

bot_state& state = bot_state::instance();

bot_state& bot_state::instance() {
  static bot_state s;
  return s;
}

void bot_state::set_lua_state(const lua::state& l) {
  l_ = l;
  q_ = std::unique_ptr<qlua::api>(new qlua::api(l));
}

void bot_state::refresh_available_instrs() {
  // Get string with all classes
  auto classes = q_->getClassesList();
  if (classes != nullptr) {
    std::string class_name;
    std::vector<std::string> all_classes;
    char* ptr = (char*)classes;
    while (strlen(ptr) > 0) {
      if (*ptr == ',') {
        all_classes.push_back(class_name);
        class_name = "";
      } else class_name += *ptr;
      ++ptr;
    }
    for (const auto& class_name : all_classes)
      refresh_available_instrs(class_name);
  }
}

void bot_state::refresh_available_instrs(const std::string& class_name) {
  try {
    auto class_secs = q_->getClassSecurities(class_name.c_str());
    char* ptr = (char*)class_secs;
    std::string sec_name;
    while (strlen(ptr) > 0) {
      if (*ptr == ',') {
        all_instrs.insert({class_name, sec_name});
        std::cout << " Instr " << class_name << "/" << sec_name << std::endl;
        sec_name = "";
      } else sec_name += *ptr;
      ++ptr;
    }
  } catch (std::exception e) {
    q_->message((std::string("l2q_spread_bot: failed to run getClassInfo for ") +
                class_name + ", exception: " + e.what()).c_str());
  }
}

void bot_state::filter_available_instrs_quik_junior() {
  auto it = all_instrs.begin();
  auto end = all_instrs.end();
  std::vector<std::string> good_names{"SBER", "LKOH", "GMKN", "ROSN", "SBERP", "MGNT", "AFLT", "BANE", "BANEP", "MTLR"};
  while (it != end) {
    if (std::find(good_names.begin(), good_names.end(), it->second) == good_names.end()) {
      all_instrs.erase(it++);
    } else {
      ++it;
    }
  }
}

void bot_state::request_bid_ask() const {
  for (const auto& instr : all_instrs) {
    try {
      const auto& class_name = instr.first.c_str();
      const auto& sec_name = instr.second.c_str();
      q_->CancelParamRequest(class_name, sec_name, "BID");
      q_->CancelParamRequest(class_name, sec_name, "OFFER");
      q_->CancelParamRequest(class_name, sec_name, "VALTODAY");
      q_->ParamRequest(class_name, sec_name, "BID");
      q_->ParamRequest(class_name, sec_name, "OFFER");
      q_->ParamRequest(class_name, sec_name, "VALTODAY");
    } catch (std::exception e) {
      q_->message((std::string("l2q_spread_bot: failed to cancel/request bid/ask parameter for ") +
                  instr.first + "/" + instr.second + ", exception: " +e.what()).c_str());
    }
  }
}

void bot_state::choose_candidates() {
  std::vector<std::pair<instrument, double>> spreads;
  
  for (const auto& instr : all_instrs) {
    try {
      bool instr_alive{false}; // Instrument has been traded today
      using param_entity = const lua::entity<lua::type_policy<qlua::table::current_trades_getParamEx>>&;
      q_->getParamEx2(instr.first.c_str(), instr.second.c_str(), "VALTODAY",
                      [this, &instr_alive] (param_entity param) {
                          if ((param().param_type() <= 2) && (param().param_value() > min_volume_))
                         instr_alive = true;
                     });
      if (instr_alive) {
        double bid{0.0};
        double ask{0.0};
        q_->getParamEx2(instr.first.c_str(), instr.second.c_str(), "BID",
                       [&bid] (param_entity param) {
                         if ((param().param_type() <= 2)) {
                           bid = param().param_value();
                         }
                       });
        q_->getParamEx2(instr.first.c_str(), instr.second.c_str(), "OFFER",
                       [&ask] (param_entity param) {
                         if ((param().param_type() <= 2)) {
                           ask = param().param_value();
                         }
                       });
        if ((ask != 0.0) && (bid != 0.0)) {
          double spread_ratio = ask/bid - double{1.0};
          //          std::cout << "got ask " << ask << " bid " << bid << " spread " << spread_ratio << std::endl;
          if (spread_ratio > min_spread_) spreads.push_back({instr, spread_ratio});
        }
      }
    } catch (std::exception e) {
      q_->message((std::string("l2q_spread_bot: failed to evaluate spread ratio for ") +
                  instr.first + "/" + instr.second + ", exception: " + e.what()).c_str());
    }
  }
  std::sort(spreads.begin(), spreads.end(), [] (const std::pair<instrument, double>& a,
                                                const std::pair<instrument, double>& b) {
              return a.second > b.second;
            });
  // Cycle through no more than num_candidates_ best instruments
  for (size_t i = 0; (i < num_candidates_) && (i < spreads.size()); ++i) {
    const auto& instr = spreads[i].first;
    auto found = instrs.find(instr);
    // If it's a new instrument or an old deactivated
    if ((found == instrs.end()) || ((found != instrs.end()) && (found->second.spread == 0))) {
      // Add it and initialize spread to info from on_param
      instrs[instr].spread = spreads[i].second;
    }
  }
  act();
  status.update();
}

void bot_state::remove_inactive_instruments() {
  std::vector<instrument> removed;
  for (const auto& ip : instrs) {
    auto& instr = ip.first;
    auto& info = ip.second;
    if ((info.bot_balance == 0) &&
        (info.buy_order.new_trans_id == 0) &&
        (info.buy_order.cancel_trans_id == 0) &&
        (info.sell_order.new_trans_id == 0) &&
        (info.sell_order.cancel_trans_id == 0) &&
        (info.l2q_subscribed == false) &&
        (info.spread == 0.0)) {
      removed.push_back(instr);
    }
  }
  for (const auto& r : removed) { instrs.erase(r); }
}

void bot_state::request_kill_order(const instrument& instr, instrument_info& info, const bool is_buy) {
  order_info* order{nullptr};
  if (is_buy) {
    order = &info.buy_order;
  } else {
    order = &info.sell_order;
  }
  // Check there's an order and there's no previous cancel request pending
  if ((order->order_key != 0) && (order->cancel_trans_id == 0)) {
    try {
      auto trans_id = next_trans_id();
      std::map<std::string, std::string> trans = {{"CLASSCODE", instr.first},
                                                  {"SECCODE", instr.second},
                                                  {"TRANS_ID", std::to_string(trans_id)},
                                                  {"ACTION", "KILLORDER"},
                                                  {"ORDER_KEY", std::to_string(order->order_key)}};
      q_->sendTransaction(trans);
      order->cancel_trans_id = trans_id;
    } catch (std::exception e) {
      q_->message((std::string("l2q_spread_bot: error sending cancel buy transaction, exception: ") + e.what()).c_str());
    }
  }
}

void bot_state::update_l2q_subscription(const instrument& instr, instrument_info& info) {
  // Consider removing unneeded subscriptions
  if (info.l2q_subscribed &&
      (info.buy_order.new_trans_id == 0) &&
      (info.buy_order.order_key == 0) &&
      (info.sell_order.new_trans_id == 0) &&
      (info.sell_order.order_key == 0) &&
      (info.bot_balance == 0) &&
      (info.spread == 0.0)) {
    if (q_->Unsubscribe_Level_II_Quotes(instr.first.c_str(), instr.second.c_str())) {
      info.l2q_subscribed = false;
    } else {
      q_->message(("Could not unsubscribe from " + instr.first + "/" + instr.second).c_str());
    }
  } else {
    // Consider adding subscriptions, if we are not subscribed yet
    if (!info.l2q_subscribed) {
      if (q_->Subscribe_Level_II_Quotes(instr.first.c_str(), instr.second.c_str())) {
        info.l2q_subscribed = true;
      } else {
        q_->message(std::string("Could not subscribe to " + instr.first + "/" + instr.second).c_str());
      }
    }
  }
}

void bot_state::act() {
  remove_inactive_instruments();
  for (auto& ip : instrs) {
    auto& instr = ip.first;
    auto& info = ip.second;
    // Kill buy order if spread became to low
    if ((info.buy_order.order_key != 0) && (info.spread < min_spread_)) {
      request_kill_order(instr, info, true);
    }
    update_l2q_subscription(instr, info);
  }
}

void bot_state::on_order(unsigned int trans_id, unsigned int order_key, const unsigned int flags, const size_t qty, const double price) {
  const instrument* instr{nullptr};
  instrument_info* info{nullptr};
  auto found = std::find_if(instrs.begin(), instrs.end(), [trans_id] (const std::pair<instrument, instrument_info>& ip) {
      const auto& info = ip.second; 
      return (info.buy_order.new_trans_id == trans_id)
      || (info.sell_order.new_trans_id == trans_id)
      || (info.buy_order.cancel_trans_id == trans_id)
      || (info.sell_order.cancel_trans_id == trans_id);
    });
  if (found != instrs.end()) {
    instr = &found->first;
    info = &found->second;
  } else {
    // Could not find by trans_id, search by order_key if it's not 0
    if (order_key != 0) {
      found = std::find_if(instrs.begin(), instrs.end(), [order_key] (const std::pair<instrument, instrument_info>& ip) {
          const auto& info = ip.second; 
          return (info.buy_order.order_key == order_key) || (info.sell_order.order_key == order_key);
        });
      if (found != instrs.end()) {
        instr = &found->first;
        info = &found->second;
      }
    }
  }
  // If order was found in bot's orders
  if (instr != nullptr) {
    order_info* order{nullptr};
    // Choose the right order object
    if (flags & 4) { // If it's a sell order
      order = &info->sell_order;
    } else { // It's a buy order
      order = &info->buy_order;
    }
    if (flags & 1) { // Active order
      order->order_key = order_key;
    } else { // Inactive order
      if (flags & 2) { // Order executed
        if (flags & 4) {
          // Sold, decrement balance
          info->bot_balance -= qty;
        } else {
          // Bought, increment balance
          info->bot_balance += qty;
        }
      }
      // Clear order info if executed/cancelled
      *order = {};
    }
    act();
  }
}

void bot_state::on_quote(const std::string& class_code, const std::string& sec_code) {
  auto found = instrs.find(instrument{class_code, sec_code});
  if (found != instrs.end()) {
    auto& instr = found->first;
    auto& info = found->second;
    // Check that instrument is in dirty state (no unfinished requests)
    if (((info.sell_order.new_trans_id != 0) && (info.sell_order.order_key == 0)) ||
        ((info.buy_order.new_trans_id != 0) && (info.buy_order.order_key == 0))) {
      // Instrument is in dirty state
    } else {
      // Instrument is not in dirty state
      try {
        q_->getQuoteLevel2(class_code.c_str(), sec_code.c_str(),
                           [this] (const ::qlua::table::level2_quotes& quotes) {
                             auto bid = quotes.bid();
                             auto offer = quotes.offer();
                             // TODO
                             act();
                           });
      } catch (std::exception e) {
        q_->message((std::string("l2q_spread_bot: error sending cancel buy transaction, exception: ") + e.what()).c_str());
      }
    }
  }
}

void bot_state::close() {
  for (auto& ip : instrs) {
    // Kill any pending orders
    if (ip.second.buy_order.order_key != 0) request_kill_order(ip.first, ip.second, true);
    if (ip.second.sell_order.order_key != 0) request_kill_order(ip.first, ip.second, false);
  }
}

unsigned int bot_state::next_trans_id() {
  ++next_trans_id_;
  return next_trans_id_;
}

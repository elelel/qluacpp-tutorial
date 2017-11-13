#include "state.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

#include <iostream>

#include "bot.hpp"
#include "status.hpp"

state::state() :
  b_(bot::instance()) {
}

void state::set_lua_state(const lua::state& l) {
  l_ = l;
  q_ = std::unique_ptr<qlua::api>(new qlua::api(l));
}

void state::refresh_available_instrs() {
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

void state::refresh_available_instrs(const std::string& class_name) {
  try {
    auto class_secs = q_->getClassSecurities(class_name.c_str());
    char* ptr = (char*)class_secs;
    std::string sec_name;
    while (strlen(ptr) > 0) {
      if (*ptr == ',') {
        all_instrs.insert({class_name, sec_name});
        sec_name = "";
      } else sec_name += *ptr;
      ++ptr;
    }
  } catch (std::exception e) {
    bot::terminate(*q_, "l2q_spread_bot: failed to run getClassInfo for " +
              class_name + ", exception: " + e.what());
  }
}

void state::filter_available_instrs_quik_junior() {
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

void state::init_client_info() {
  try {
    auto n = q_->getNumberOf<::qlua::table::client_codes>();
    if (n < 1) {
      bot::terminate(*q_, "l2q_spread_bot: too few client codes!");
    } else {
      using client_code_entity = const lua::entity<lua::type_policy<qlua::table::client_codes>>&;
      using trade_account_entity = const lua::entity<lua::type_policy<qlua::table::trade_accounts>>&;
      q_->getItem<::qlua::table::client_codes>(0, [this] (client_code_entity c) {
          client_code = c();
        });
      n = q_->getNumberOf<::qlua::table::trade_accounts>();
      for (size_t i = 0; i < n; ++i) {
        q_->getItem<::qlua::table::trade_accounts>(i, [this] (trade_account_entity e) {
            auto trdaccid = e().trdaccid();
            auto class_codes = e().class_codes();
            std::string class_name;
            while (class_codes[0] == '|') class_codes = class_codes.substr(1, class_codes.size() - 1);
            size_t start = 0;
            for (size_t k = 0; k < class_codes.size(); ++k) {
              if (class_codes[k] == '|') {
                if (k > start) {
                  class_name = class_codes.substr(start, k - start);
                  start = k + 1;
                  class_to_accid[class_name] = trdaccid;
                }
              }
            }
          });
      }
    }
  } catch (std::exception e) {
    bot::terminate(*q_, "l2q_spread_bot: failed to get client info for, exception: " + std::string(e.what()));
  }
}

void state::request_bid_ask() {
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
      bot::terminate(*q_, "l2q_spread_bot: failed to cancel/request bid/ask parameter for " +
        instr.first + "/" + instr.second + ", exception: " +e.what());
    }
  }
}

void state::choose_candidates() {
  using param_entity = const lua::entity<lua::type_policy<qlua::table::current_trades_getParamEx>>&;

  std::vector<std::pair<instrument, double>> spreads;
  for (const auto& instr : all_instrs) {
    try {
      bool instr_alive{false}; // Instrument has been traded today
      q_->getParamEx2(instr.first.c_str(), instr.second.c_str(), "VALTODAY",
                      [this, &instr_alive] (param_entity param) {
                        if ((param().param_type() <= 2) && (param().param_value() > b_.settings().min_volume))
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
          if (spread_ratio > b_.settings().min_spread) spreads.push_back({instr, spread_ratio});
        }
      }
    } catch (std::exception e) {
      bot::terminate(*q_, "l2q_spread_bot: failed to evaluate spread ratio for " +
                   instr.first + "/" + instr.second + ", exception: " + e.what());
    }
  }
  std::sort(spreads.begin(), spreads.end(), [] (const std::pair<instrument, double>& a,
                                                const std::pair<instrument, double>& b) {
              return a.second > b.second;
            });
  // Cycle through no more than num_candidates best instruments
  for (size_t i = 0; (i < b_.settings().num_candidates) && (i < spreads.size()); ++i) {
    const auto& instr = spreads[i].first;
    auto found = instrs.find(instr);
    // If it's a new instrument or an old deactivated
    if ((found == instrs.end()) || ((found != instrs.end()) && (found->second.spread == 0) && (!found->second.l2q_subscribed))) {
      // Add it and initialize spread to info from on_param
      instrs[instr].spread = spreads[i].second;
      q_->getParamEx2(instr.first.c_str(), instr.second.c_str(), "SEC_PRICE_STEP",
                      [this, &instr] (param_entity param) {
                        instrs[instr].sec_price_step = param().param_value();
                      });
    }
  }
  act();
}

void state::remove_inactive_instruments() {
  std::vector<instrument> removed;
  for (const auto& ip : instrs) {
    auto& instr = ip.first;
    auto& info = ip.second;
    if ((info.balance == 0) &&
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

void state::update_l2q_subscription(const instrument& instr, instrument_info& info) {
  // Consider removing unneeded subscriptions
  if (info.l2q_subscribed &&
      (info.buy_order.new_trans_id == 0) &&
      (info.buy_order.order_key == 0) &&
      (info.sell_order.new_trans_id == 0) &&
      (info.sell_order.order_key == 0) &&
      (info.balance == 0) &&
      (info.spread < b_.settings().min_spread)) {
    // Sometimes Quik for some unknown reason fails to unsubscribe. Let's check that it thinks we are subscribed first
    if (q_->IsSubscribed_Level_II_Quotes(instr.first.c_str(), instr.second.c_str())) {
      if (q_->Unsubscribe_Level_II_Quotes(instr.first.c_str(), instr.second.c_str())) {
        info.l2q_subscribed = false;
        info.spread = 0;
        info.sell_order.estimated_price = 0;
        info.buy_order.estimated_price = 0;
      } else {
        q_->message(("l2q_spread_bot: could not unsubscribe from " + instr.first + "/" + instr.second).c_str());
      }
    } else {
      bot::terminate(*q_, "We are subscribed to " + instr.first + "/" + instr.second + ", but Quik doesn't think so!");
    }
  } else {
    // Consider adding subscriptions, if we are not subscribed yet
    if (!info.l2q_subscribed) {
      if (q_->Subscribe_Level_II_Quotes(instr.first.c_str(), instr.second.c_str())) {
        info.l2q_subscribed = true;
      } else {
        bot::terminate(*q_, "Could not subscribe to " + instr.first + "/" + instr.second);
      }
    }
  }
}

void state::request_new_order(const instrument& instr, const instrument_info& info,
                                  order_info& order, const std::string& operation, const size_t qty) {
  // Check protective time limits
  try {
    // Calculate decimal precision for the price
    std::string price_step = std::to_string(info.sec_price_step);
    auto pos = std::string::npos;
    pos = price_step.find_last_of('.');
    if (pos != std::string::npos) price_step = price_step.substr(pos + 1, price_step.size());
    pos = price_step.find_last_of(',');
    if (pos != std::string::npos) price_step = price_step.substr(pos + 1, price_step.size());
    while ((price_step.size() > 0) && (price_step[price_step.size() - 1] == '0')) price_step.pop_back();
    auto precision = price_step.size();

    auto price = order.estimated_price;


    std::string price_s;
    if (precision > 0) {
      std::stringstream price_ss;
      price_ss << std::fixed;
      price_ss << std::setprecision(precision);
      price_ss << price;
      price_s = price_ss.str();
    } else {
      price_s = std::to_string(int(price));
    }
    
    auto buy_sell = q_->CalcBuySell(instr.first.c_str(), instr.second.c_str(),
                                    client_code.c_str(), class_to_accid[instr.first].c_str(),
                                    price, true, false);
    const auto& max_qty = std::get<0>(buy_sell);
    std::cout << " est price " << order.estimated_price
              << " sec_price_step " << info.sec_price_step
              << " precision " << precision
              << " qty " << qty
              << " max qty " << max_qty
              << " comission " << std::get<1>(buy_sell)
              << " price_s " << price_s     
              << " price " << price << std::endl;
    if ((qty > 0) && (qty <= max_qty) && (price != 0.0)) {
      auto trans_id = next_trans_id();

      std::map<std::string, std::string> trans = {
        {"ACCOUNT", class_to_accid[instr.first].c_str()},
        {"CLIENT_CODE", client_code.c_str()},
        {"CLASSCODE", instr.first},
        {"SECCODE", instr.second},
        {"TRANS_ID", std::to_string(trans_id)},
        {"QUANTITY", std::to_string(qty)},
        {"OPERATION", operation.c_str()},
        {"PRICE", price_s},
        {"ACTION", "NEW_ORDER"}
      };
      q_->sendTransaction(trans);
      order.new_trans_id = trans_id;
      order.placed_price = price;
      // Save transaction time for time limit
      trans_times_within_hour.push_back(std::chrono::time_point<std::chrono::steady_clock>());
    }
  } catch (std::exception e) {
    bot::terminate(*q_, "l2q_spread_bot: error calculating/sending buy, exception: " + std::string(e.what()));
  }
}

void state::request_kill_order(const instrument& instr, order_info& order) {
  // Check there's an order and there's no previous cancel request pending
  if ((order.order_key != 0) && (order.cancel_trans_id == 0)) {
    try {
      auto trans_id = next_trans_id();
      std::map<std::string, std::string> trans = {
        {"CLASSCODE", instr.first},
        {"SECCODE", instr.second},
        {"TRANS_ID", std::to_string(trans_id)},
        {"ACTION", "KILL_ORDER"},
        {"ORDER_KEY", std::to_string(order.order_key)}
      };
      q_->sendTransaction(trans);
      order.cancel_trans_id = trans_id;
      // Save transaction time for time limit
      trans_times_within_hour.push_back(std::chrono::time_point<std::chrono::steady_clock>());
    } catch (std::exception e) {
      bot::terminate(*q_, "l2q_spread_bot: error sending cancel buy transaction, exception: " + std::string(e.what()));
    }
  }
}
  
bool state::trans_times_limits_ok() {
  auto now = std::chrono::time_point<std::chrono::steady_clock>();
  while (trans_times_within_hour.front() < now - std::chrono::hours{1})
    trans_times_within_hour.pop_front();
  return trans_times_within_hour.size() < 50;
}

void state::act() {
  remove_inactive_instruments();
  for (auto& ip : instrs) {
    auto& instr = ip.first;
    auto& info = ip.second;
    // If we have an active buy order
    if ((info.buy_order.order_key != 0)) {
      // Kill buy order if spread became low
      if (info.spread < b_.settings().min_spread) {
        request_kill_order(instr, info.buy_order);
      }
    }
    // If we have an active sell order
    if ((info.sell_order.order_key != 0)) {
      // Kill sell order if spread became low
      if (info.spread < b_.settings().min_spread) {
        request_kill_order(instr, info.sell_order);
      }
    }

    // Do we need to place a new buy order?
    if ((info.buy_order.order_key == 0) &&
        (info.spread >= b_.settings().min_spread) &&
        (info.buy_order.estimated_price != 0.0) &&
        (info.buy_order.new_trans_id == 0) && (info.buy_order.cancel_trans_id == 0)) {
      const auto qty = b_.settings().my_order_size - info.balance;
      if (qty > 0) {
        std::cout << "REQUESTING BUY " << instr.second << std::endl;
        request_new_order(instr, info, info.buy_order, "B", qty);
      }
    }
    // Do we need to place a new sell order?
    if ((info.sell_order.order_key == 0) &&
        (info.spread >= b_.settings().min_spread) &&
        (info.sell_order.estimated_price != 0.0) &&
        (info.sell_order.new_trans_id == 0) && (info.sell_order.cancel_trans_id == 0)) {
      const auto qty = info.balance;
      if ((qty > 0) && (trans_times_limits_ok())) {
        request_new_order(instr, info, info.sell_order, "S", qty);
      }
    }
    
    update_l2q_subscription(instr, info);
  }
  b_.update_status = true;
}

void state::on_order(unsigned int trans_id, unsigned int order_key, const unsigned int flags, const size_t qty, const size_t balance, const double price) {
  const instrument* instr{nullptr};
  instrument_info* info{nullptr};

  if (trans_id != 0) {
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
    }
  }

  if ((instr == nullptr) && (order_key != 0)) {
    // Could not find by trans_id, search by order_key if it's not 0
    auto found = std::find_if(instrs.begin(), instrs.end(), [order_key] (const std::pair<instrument, instrument_info>& ip) {
        const auto& info = ip.second; 
        return (info.buy_order.order_key == order_key) || (info.sell_order.order_key == order_key);
      });
    if (found != instrs.end()) {
      instr = &found->first;
      info = &found->second;
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
    if (!(flags & 1) && !(flags & 2)) { // Executed
      if (flags & 4) {
        std::cout << "ORDER sold" << std::endl;
        // Sold, decrement balance
        info->balance -= qty - balance;
      } else {
        std::cout << "ORDER bought" << std::endl;
        // Bought, increment balance
        info->balance_price = (info->balance * info->balance_price + qty * price) /
          (info->balance + qty);
        info->balance += qty - balance;
      }
      *order = {};
    }
    else if (flags & 2) { // Removed
      *order = {};
    }
    else if (flags & 1) { // Active
      order->order_key = order_key;
    } else if (!(flags &1)) { // Inactive
      order->order_key = 0;
    }
    act();
  }
}

void state::on_quote(const std::string& class_code, const std::string& sec_code) {
  auto found = instrs.find(instrument{class_code, sec_code});
  if (found != instrs.end()) {
    auto& instr = found->first;
    auto& info = found->second;
    // Check that instrument is in dirty state (no unfinished requests)
    bool dirty{false};
    // Sent sell order, but no order number yet -> dirty
    if ((info.sell_order.new_trans_id != 0) && (info.sell_order.order_key == 0)) dirty = true;
    // Sent buy order, but no order number yet -> dirty
    if ((info.buy_order.new_trans_id != 0) && (info.buy_order.order_key == 0)) dirty = true;
    if (!dirty) {
      try {
        q_->getQuoteLevel2(class_code.c_str(), sec_code.c_str(),
                           [this, &instr, &info] (const ::qlua::table::level2_quotes& quotes) {
                             auto bid = quotes.bid();
                             auto offer = quotes.offer();

                             double acc{0.0};
                             double my_bid_price{0.0};
                             double my_ask_price{0.0};
                             if (bid.size() > 0) {
                               size_t i{0};
                               for (i = 0; i < bid.size(); ++i) {
                                 const auto& rec = bid[bid.size() - 1 - i];
                                 const double qty = atof(rec.quantity.c_str());
                                 const double price = atof(rec.price.c_str());
                                 auto new_acc = acc + qty * price;
                                 if (info.buy_order.order_key != 0) {
                                   // Don't count our own order
                                   new_acc -= info.buy_order.qty * info.buy_order.placed_price;
                                 }
                                 my_bid_price = atof(rec.price.c_str()) + info.sec_price_step;
                                 //                                 std::cout << "  Instr " << instr.first << "/" << instr.second
                                 //        << " qty " << qty << " price " << price << " new_acc " << new_acc << " my b p " << my_bid_price << std::endl;
                                 if (new_acc > (b_.settings().my_order_size * my_bid_price * b_.settings().vol_ignore_coeff)) {
                                   break;
                                 } else {
                                   acc = new_acc;
                                 }
                               }
                             }
                             //                             std::cout << "My bid price " << my_bid_price << ::std::endl;

                             acc = 0.0;
                             if (offer.size() > 0) {
                               size_t i{0};
                               for (i = 0; i < offer.size(); ++i) {
                                 const auto& rec = offer[i];
                                 const double qty = atof(rec.quantity.c_str());
                                 const double price = atof(rec.price.c_str());
                                 auto new_acc = acc + qty * price;
                                 if (info.sell_order.order_key != 0) {
                                   // Don't count our own order
                                   new_acc -= info.sell_order.qty * info.sell_order.placed_price;
                                 }
                                 my_ask_price = atof(rec.price.c_str()) - info.sec_price_step;
                                 if (new_acc >
                                     (b_.settings().my_order_size * my_ask_price * b_.settings().vol_ignore_coeff)) {
                                   break;
                                 } else {
                                   acc = new_acc;
                                 }
                               }
                             }

                             if ((my_bid_price > 0) && (my_ask_price > 0)) {
                               info.buy_order.estimated_price = round(my_bid_price / info.sec_price_step) * info.sec_price_step;
                               info.sell_order.estimated_price = round(my_ask_price / info.sec_price_step) * info.sec_price_step;
                               info.spread = my_ask_price/my_bid_price - 1.0;
                               //                               std::cout << "Set buy " << my_bid_price << " sell " << my_ask_price << " spread " << info.spread << std::endl;
                             } else {
                               info.buy_order.estimated_price = 0;
                               info.sell_order.estimated_price = 0;
                               info.spread = 0;
                             }
                             act();
                             b_.update_status = true;
                           });
      } catch (std::exception e) {
        bot::terminate(*q_, "l2q_spread_bot: error sending cancel buy transaction, exception: " + std::string(e.what()));
      }
    }
  }
}

void state::on_stop() {
  for (auto& ip : instrs) {
    // Kill any pending orders
    if (ip.second.buy_order.order_key != 0) request_kill_order(ip.first, ip.second.buy_order);
    if (ip.second.sell_order.order_key != 0) request_kill_order(ip.first, ip.second.sell_order);
  }
}

unsigned int state::next_trans_id() {
  ++next_trans_id_;
  return next_trans_id_;
}

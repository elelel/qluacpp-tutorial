#include "status.hpp"

bot_status& status = bot_status::instance();

bot_status& bot_status::instance() {
  static bot_status s;
  return s;
}

void bot_status::set_lua_state(const lua::state& l) {
  l_ = l;
  q_ = std::unique_ptr<qlua::api>(new qlua::api(l));
}

void bot_status::create_window() {
  try {
    table_id_ = q_->AllocTable();
    if (table_id_ != 0) {
      try {
        auto string_type = q_->constant<unsigned int>("QTABLE_STRING_TYPE");
        int rslt{1};
        rslt = rslt & q_->AddColumn(table_id_, column::CLASS, "Class", true, string_type, 8);
        rslt = rslt & q_->AddColumn(table_id_, column::NAME, "Name", true, string_type, 8);
        rslt = rslt & q_->AddColumn(table_id_, column::SEC_PRICE_STEP, "Price step", true, string_type, 12);
        rslt = rslt & q_->AddColumn(table_id_, column::BALANCE, "Balance", true, string_type, 8);
        rslt = rslt & q_->AddColumn(table_id_, column::BUY_PRICE, "Buy price", true, string_type, 12);
        rslt = rslt & q_->AddColumn(table_id_, column::SELL_PRICE, "Sell price", true, string_type, 12);
        rslt = rslt & q_->AddColumn(table_id_, column::SPREAD, "Spread", true, string_type, 10);
        rslt = rslt & q_->AddColumn(table_id_, column::BUY_ORDER, "Buy order", true, string_type, 20);
        rslt = rslt & q_->AddColumn(table_id_, column::SELL_ORDER, "Sell order", true, string_type, 20);
        if (rslt == 1) {
          try {
            if (q_->CreateWindow(table_id_)) {
              q_->SetWindowCaption(table_id_, "qluacpp l2q_spread_bot bot_status");
              window_created_ = true;
            }
          } catch (std::exception e) {
            q_->message((std::string("Failed to CreateWindow/SetWindowCaption, exception: ") + e.what()).c_str());
          }
        } else {
          q_->message("Failed to AddColumn, returned false");
        }
      } catch (std::exception e) {
        q_->message((std::string("Failed to AddColumn, exception: ") + e.what()).c_str());
      }
    } else {
      q_->message("Failed to AllocTable, returned 0 id");
    }
  } catch (std::exception e) {
    q_->message((std::string("Failed to AllocTable, exception: ") + e.what()).c_str());
  }
}

void bot_status::update() {
  if (window_created_) {
    try {
      auto def_color = q_->constant<int>("QTABLE_DEFAULT_COLOR");
      q_->Clear(table_id_);
      int n{-1};
      for (const auto& ip : state.instrs) {
        n = q_->InsertRow(table_id_, -1);
        const auto& info = ip.second;
        q_->SetCell(table_id_, n, column::CLASS, ip.first.first.c_str(), 0);
        q_->SetCell(table_id_, n, column::NAME, ip.first.second.c_str(), 0);
        if (info.l2q_subscribed) {
          q_->SetColor(table_id_, n, column::CLASS, 0xffffff, def_color, def_color, def_color);
          q_->SetColor(table_id_, n, column::NAME, 0xffffff, def_color, def_color, def_color);
        } else {
          q_->SetColor(table_id_, n, column::CLASS, 0xffffff, 0xcccccc, def_color, def_color);
          q_->SetColor(table_id_, n, column::NAME, 0xffffff, 0xcccccc, def_color, def_color);
        }
        q_->SetCell(table_id_, n, column::SEC_PRICE_STEP, std::to_string(info.sec_price_step).c_str(), 0);
        q_->SetCell(table_id_, n, column::BALANCE, std::to_string(info.bot_balance).c_str(), 0);
        int buy_color{0xcccccc};
        if (info.buy_order.order_key != 0) buy_color = 0x00ff22; else
          if (info.spread > state.min_spread) buy_color = 0x000000;
        q_->SetCell(table_id_, n, column::BUY_PRICE, std::to_string(info.buy_order.price).c_str(), 0);
        q_->SetColor(table_id_, n, column::BUY_PRICE, 0xffffff, buy_color, def_color, def_color);
        int sell_color{0xcccccc};
        if (info.sell_order.order_key != 0) sell_color = 0x00ff22; else
          if (info.spread > state.min_spread) sell_color = 0x000000;
        q_->SetCell(table_id_, n, column::SELL_PRICE, std::to_string(info.sell_order.price).c_str(), 0);
        q_->SetColor(table_id_, n, column::SELL_PRICE, 0xffffff, sell_color, def_color, def_color);
        q_->SetCell(table_id_, n, column::SPREAD, std::to_string(info.spread).c_str(), 0);
        
        if (info.buy_order.order_key != 0) {
          q_->SetCell(table_id_, n, column::BUY_ORDER, std::to_string(info.buy_order.order_key).c_str(), 0);
        } else {
          q_->SetCell(table_id_, n, column::BUY_ORDER, "", 0);
        }
        if (info.sell_order.order_key != 0) {
          q_->SetCell(table_id_, n, column::SELL_ORDER, std::to_string(info.sell_order.order_key).c_str(), 0);
        } else {
          q_->SetCell(table_id_, n, column::SELL_ORDER, "", 0);
        }
      }
    } catch (std::exception e) {
      q_->message((std::string("Failed to update bot_status, exception: ") + e.what()).c_str());
    }
  }
}

void bot_status::close() {
  if (window_created_) {
    try {
      if (!q_->DestroyTable(table_id_)) {
        q_->message("Failed to DestroyTable, returned false");
      }
    } catch (std::exception e) {
      q_->message((std::string("Failed to DestroyTable, exception: ") + e.what()).c_str());
    }
  }
}

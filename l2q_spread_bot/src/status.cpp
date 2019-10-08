#include "status.hpp"

#include <thread>

#include "bot.hpp"

status::status() :
  b_(bot::instance()) {
}

void status::set_lua_state(const lua::state& l) {
  l_ = l;
  q_ = std::unique_ptr<qlua::extended>(new qlua::extended(l_));
  if (table_id_ == 0) {
    create_window();
    update_title();
  }
}

status::~status() {
  if (table_id_ != 0) {
    try {
      std::cout << "Calling destroy table" << std::endl;
      try {
        if (!q_->DestroyTable(table_id_)) {
          q_->message("Failed to DestroyTable, returned false");
        }
      } catch (std::exception e) {
        q_->message("Failed to DestroyTable, exception: " + std::string(e.what()));
      }
      table_id_ = 0;
    } catch (std::exception e) {
      q_->message((std::string("Failed to DestroyTable, exception: ") + e.what()).c_str());
    }
  }
  std::cout << "Status destructor done" << std::endl;
}

void status::create_window() {
  try {
    table_id_ = q_->AllocTable();
    if (table_id_ != 0) {
      try {
        auto string_type = q_->constant<unsigned int>("QTABLE_STRING_TYPE");
        int rslt{1};
        rslt = rslt & q_->AddColumn(table_id_, column::ACCID, "Account", true, string_type, 20);
        rslt = rslt & q_->AddColumn(table_id_, column::CLASS, "Class", true, string_type, 8);
        rslt = rslt & q_->AddColumn(table_id_, column::NAME, "Name", true, string_type, 8);
        rslt = rslt & q_->AddColumn(table_id_, column::SEC_PRICE_STEP, "Price step", true, string_type, 12);
        rslt = rslt & q_->AddColumn(table_id_, column::BALANCE, "Balance", true, string_type, 8);
        rslt = rslt & q_->AddColumn(table_id_, column::EST_BUY_PRICE, "Est BUY", true, string_type, 12);
        rslt = rslt & q_->AddColumn(table_id_, column::EST_SELL_PRICE, "Est SELL", true, string_type, 12);
        rslt = rslt & q_->AddColumn(table_id_, column::PLACED_BUY_PRICE, "Placed BUY", true, string_type, 12);
        rslt = rslt & q_->AddColumn(table_id_, column::PLACED_SELL_PRICE, "Placed SELL", true, string_type, 12);
        rslt = rslt & q_->AddColumn(table_id_, column::SPREAD, "Spread", true, string_type, 10);
        rslt = rslt & q_->AddColumn(table_id_, column::BUY_ORDER, "Buy order", true, string_type, 20);
        rslt = rslt & q_->AddColumn(table_id_, column::SELL_ORDER, "Sell order", true, string_type, 20);
        if (rslt == 1) {
          try {
            if (q_->CreateWindow(table_id_)) {
              q_->SetWindowCaption(table_id_, "QLuaCPP spread bot example");
            }
          } catch (std::exception e) {
            bot::terminate(*q_, "Failed to CreateWindow/SetWindowCaption, exception: " + std::string(e.what()));
          }
        } else {
          bot::terminate(*q_, "Failed to AddColumn, returned false");
        }
      } catch (std::exception e) {
        bot::terminate(*q_, "Failed to AddColumn, exception: " + std::string(e.what()));
      }
    } else {
      bot::terminate(*q_, "Failed to AllocTable, returned 0 id");
    }
  } catch (std::exception e) {
    bot::terminate(*q_, "Failed to AllocTable, exception: " + std::string(e.what()));
  }
}

void status::update_title() {
  q_->SetWindowCaption(table_id_, ("QLuaCPP spread bot example - client: " + b_.get_state()->client_code).c_str());
}

void status::update() {
  return;  // Do not show status; new Quik versions changed thread-locking policy
  
  if (table_id_ != 0) {
    try {
      auto def_color = q_->constant<int>("QTABLE_DEFAULT_COLOR");
      std::cout << "Calling first UI QLUA func (Clear) in status::update. This will lock if called from incorrect thread." << std::endl;
      q_->Clear(table_id_);
      std::cout << "First UI Qlua func returned" << std::endl;
      int n{-1};
      for (const auto& ip : b_.get_state()->instrs) {
        std::cout << "  update cycle: Calling InsertRow. This will lock on Quik >8.0" << std::endl;
        n = q_->InsertRow(table_id_, -1);
        std::cout << "  update cycle: Calling SetCell" << std::endl;
        const auto& info = ip.second;
        q_->SetCell(table_id_, n, column::ACCID, b_.get_state()->class_to_accid[ip.first.first].c_str(), 0);
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
        q_->SetCell(table_id_, n, column::BALANCE, std::to_string(info.balance).c_str(), 0);
        int buy_color{0xcccccc};
        if (info.buy_order.order_key != 0) buy_color = 0x00ff22; else
          if ((info.buy_order.estimated_price != 0) && (info.spread > b_.settings().min_spread)) buy_color = 0x000000;
        q_->SetCell(table_id_, n, column::EST_BUY_PRICE, std::to_string(info.buy_order.estimated_price).c_str(), 0);
        q_->SetColor(table_id_, n, column::EST_BUY_PRICE, 0xffffff, buy_color, def_color, def_color);
        int sell_color{0xcccccc};
        if (info.sell_order.order_key != 0) sell_color = 0x00ff22; else
          if ((info.buy_order.estimated_price != 0) && (info.spread > b_.settings().min_spread)) sell_color = 0x000000;
        q_->SetCell(table_id_, n, column::EST_SELL_PRICE, std::to_string(info.sell_order.estimated_price).c_str(), 0);
        q_->SetColor(table_id_, n, column::EST_SELL_PRICE, 0xffffff, sell_color, def_color, def_color);
        q_->SetCell(table_id_, n, column::SPREAD, std::to_string(info.spread).c_str(), 0);
        
        if (info.buy_order.order_key != 0) {
          q_->SetCell(table_id_, n, column::BUY_ORDER, std::to_string(info.buy_order.order_key).c_str(), 0);
          q_->SetCell(table_id_, n, column::PLACED_BUY_PRICE, std::to_string(info.buy_order.placed_price).c_str(), 0);
        } else {
          q_->SetCell(table_id_, n, column::BUY_ORDER, "", 0);
          q_->SetCell(table_id_, n, column::PLACED_BUY_PRICE, "", 0);
        }
        if (info.sell_order.order_key != 0) {
          q_->SetCell(table_id_, n, column::SELL_ORDER, std::to_string(info.sell_order.order_key).c_str(), 0);
          q_->SetCell(table_id_, n, column::PLACED_SELL_PRICE, std::to_string(info.sell_order.placed_price).c_str(), 0);
        } else {
          q_->SetCell(table_id_, n, column::SELL_ORDER, "", 0);
          q_->SetCell(table_id_, n, column::PLACED_SELL_PRICE, "", 0);
        }
      }
      std::cout << "Status update done " << std::endl;
    } catch (std::exception e) {
      q_->message((std::string("Failed to update bot_status, exception: ") + e.what()).c_str());
    }
  }
}


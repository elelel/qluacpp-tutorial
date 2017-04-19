#pragma once

#include <map>
#include <string>

#include <qlua>

struct order_db {
  order_db(const std::string& filename);

  // Receive order information from periodic orders table check
  // or from OnOrder callback
  void update(qlua::extended_api& q, const qlua::table::row::orders& orders);
  // Update order information by order num
  void update(qlua::extended_api& q, const int& order_num);
  
private:
  std::string filename_;

  // Convert order information to .tri format
  std::map<std::string, std::string> to_tri(const qlua::table::row::orders& orders);
};

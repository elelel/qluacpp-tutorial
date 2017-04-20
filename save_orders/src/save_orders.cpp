#define LUA_LIB
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
  #define LUA_BUILD_AS_DLL
#endif

#include <chrono>
#include <memory>
#include <thread>

#include <iostream>

#include <qlua>

#include "log_window.hpp"
#include "order_db.hpp"

static struct luaL_reg ls_lib[] = {
  { NULL, NULL }
};

static std::shared_ptr<log_window> log_;
static order_db db("save_orders.json");

void my_main(lua::state& l) {
  using namespace std::chrono_literals;
  qlua::extended_api q(l);
  log_ = std::make_shared<log_window>(q);
  db.set_log_window(log_);
  while (true) {
    log_->invoke_on_main();
    std::this_thread::sleep_for(200ms);
  }
}

void OnOrder(lua::state& l, const qlua::table::row::orders& order) {
  log_->add("Received OnOrder event");
  qlua::extended_api q(l);
  db.update(q, order);
}

void OnTrade(lua::state& l, const qlua::trade& trade) {
  log_->add("Received OnTrade event");
  qlua::extended_api q(l);
  db.update(q, trade.class_code, trade.order_num);
}

extern "C" {
  LUALIB_API int luaopen_lualib_save_orders(lua_State *L) {
    lua::state l(L);
    qlua::extended_api q(l);

    q.set_callback<qlua::callback::OnOrder>(OnOrder);
    //    q.set_callback<qlua::callback::OnTrade>(OnTrade);
    q.set_callback<qlua::callback::main>(my_main);

    luaL_openlib(L, "lualib_save_orders", ls_lib, 0);
    return 0;
  }
}


#define LUA_LIB
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
  #define LUA_BUILD_AS_DLL
#endif

#include <chrono>
#include <thread>

#include <qluacpp/qlua>

#include "trade_logger.hpp"

#include <iostream>

static struct luaL_reg ls_lib[] = {
  { NULL, NULL }
};

void my_main(lua::state& l) {
  using namespace std::chrono_literals;
  while (true) {
    std::this_thread::sleep_for(1s);
  }
}

void OnAllTrade(const lua::state& l,
                ::lua::entity<::lua::type_policy<::qlua::table::all_trades>> data) {
  // Create log record in our format
  log_record rec;

  // Record record creation time
  rec.time = std::chrono::system_clock::now();
  // Copy data from callback
  rec.class_code = data().class_code();
  rec.sec_code = data().sec_code();
  rec.price = data().price();
  rec.value = data().value();
  rec.qty = data().qty();
  // Request additional data on instrument from QLua
  qlua::api q(l);
  q.getSecurityInfo(rec.class_code.c_str(), rec.sec_code.c_str(),
                    [&rec] (const lua::state& s) {
                      auto sec_info = s.at<::qlua::table::securities>(-1);
                      rec.name = sec_info().name();
                      return 1;  // How many stack items should be cleaned up (poped)
                    });
  // Send the record to trade logger
  trade_logger::instance().update(rec);
}

std::tuple<int> OnStop(const lua::state& l,
           ::lua::entity<::lua::type_policy<int>> signal) {
  return std::make_tuple(int(1));
}

LUACPP_STATIC_FUNCTION2(main, my_main)
LUACPP_STATIC_FUNCTION3(OnAllTrade, OnAllTrade, ::qlua::table::all_trades)
LUACPP_STATIC_FUNCTION3(OnStop, OnStop, int)

extern "C" {
  LUALIB_API int luaopen_lualib_log_all_trades(lua_State *L) {
    lua::state l(L);

    ::lua::function::main().register_in_lua(l, my_main);
    ::lua::function::OnAllTrade().register_in_lua(l, OnAllTrade);
    ::lua::function::OnStop().register_in_lua(l, OnStop);

    luaL_openlib(L, "lualib_log_all_trades", ls_lib, 0);
    return 0;
  }
}

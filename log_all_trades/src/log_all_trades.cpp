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
  trade_logger::instance().update(data);
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

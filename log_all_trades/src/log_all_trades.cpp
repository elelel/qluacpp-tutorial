#define LUA_LIB
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
  #define LUA_BUILD_AS_DLL
#endif

#include <chrono>
#include <thread>

#include <qlua>

#include "trade_logger.hpp"

static struct luaL_reg ls_lib[] = {
  { NULL, NULL }
};

void my_main(lua::state& l) {
  using namespace std::chrono_literals;
  qlua::extended_api q(l);
  for (int i = 0; i < 5; ++i) {
    std::this_thread::sleep_for(1s);
  }
}

void OnAllTrade(lua::state& l, const qlua::alltrade& data) {
  trade_logger::instance().update(data);
}

extern "C" {
  LUALIB_API int luaopen_lualib_log_all_trades(lua_State *L) {
    lua::state l(L);
    qlua::extended_api q(l);

    q.set_callback<qlua::callback::main>(my_main);
    q.set_callback<qlua::callback::OnAllTrade>(OnAllTrade);

    luaL_openlib(L, "lualib_log_all_trades", ls_lib, 0);
    return 0;
  }
}

#define LUA_LIB
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
  #define LUA_BUILD_AS_DLL
#endif

#include <chrono>
#include <memory>
#include <mutex>
#include <thread>

#include <qluacpp/api>

#include "bot.hpp"

static struct luaL_reg ls_lib[] = {
  { NULL, NULL }
};

LUACPP_STATIC_FUNCTION2(main, bot::main)
LUACPP_STATIC_FUNCTION4(thread_safe_main, bot::thread_safe_main, const int, const int)
LUACPP_STATIC_FUNCTION3(OnStop, bot::on_stop, int)
LUACPP_STATIC_FUNCTION3(OnOrder, bot::on_order, ::qlua::table::orders)
LUACPP_STATIC_FUNCTION4(OnQuote, bot::on_quote, const char*, const char*)
LUACPP_STATIC_FUNCTION3(OnTransReply, bot::on_trans_reply, ::qlua::table::trans_reply)
                        
extern "C" {
  LUALIB_API int luaopen_lualib_l2q_spread_bot(lua_State *L) {
    lua::state l(L);

    ::lua::function::main().register_in_lua(l, bot::main);
    ::lua::function::thread_safe_main().register_in_lua(l, bot::thread_safe_main);
    ::lua::function::OnStop().register_in_lua(l, bot::on_stop);
    ::lua::function::OnOrder().register_in_lua(l, bot::on_order);
    ::lua::function::OnQuote().register_in_lua(l, bot::on_quote);
    ::lua::function::OnTransReply().register_in_lua(l, bot::on_trans_reply);

    luaL_openlib(L, "lualib_l2q_spread_bot", ls_lib, 0);
    return 0;
  }
}

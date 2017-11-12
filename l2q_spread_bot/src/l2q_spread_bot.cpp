#define LUA_LIB
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
  #define LUA_BUILD_AS_DLL
#endif

#include <chrono>
#include <mutex>
#include <thread>

#include <qluacpp/qlua>

#include "state.hpp"
#include "status.hpp"
#include "sync.hpp"

static struct luaL_reg ls_lib[] = {
  { NULL, NULL }
};

void my_main(lua::state& l) {
  using namespace std::chrono_literals;
  qlua::api q(l);
  {
    lock_guard lock(mutex);
    state.set_lua_state(l);
    state.refresh_available_instrs();
    state.filter_available_instrs_quik_junior(); // For trading on Quik Junior emulator, remove otherwise
    state.init_client_info();
    state.request_bid_ask();
    // Initialize status statics
    bot_status status(l);
  }

  int candidates_timer{0};
  while (!state.terminated) {
    // Emulate timers
    // Check periodically if we have new instrument candidates
    if (candidates_timer <= 0) {
      std::cout << "Main cv" << std::endl;
      {
        lock_guard lock(mutex);
        state.set_lua_state(l);
        state.choose_candidates();
      }
      std::cout << "Main done" << std::endl;
      candidates_timer = 5; // Reset timer to fire again
    }
    std::this_thread::sleep_for(1s);
    --candidates_timer;
  }
  std::this_thread::sleep_for(1s);
}


std::tuple<int> OnStop(const lua::state& l,
                       ::lua::entity<::lua::type_policy<int>> signal) {
    std::cout << "OnStop " << std::endl;
    {
      lock_guard lock(mutex);
      state.set_lua_state(l);
      state.on_stop();
    }
    std::cout << "OnStop done" << std::endl;
    return std::make_tuple(int(1));
}

void OnOrder(const lua::state& l,
             ::lua::entity<::lua::type_policy<::qlua::table::orders>> order) {
    std::cout << "OnOrder" << std::endl;
    {
      lock_guard lock(mutex);
      state.set_lua_state(l);
      state.on_order(order().trans_id(),
                     order().order_num(),
                     order().flags(),
                     order().qty(),
                     order().balance(),
                     order().price());
    }
    std::cout << "OnOrder done" << std::endl;
}

void OnQuote(const lua::state& l,
             ::lua::entity<::lua::type_policy<const char*>> sec_class,
             ::lua::entity<::lua::type_policy<const char*>> sec_code) {
  std::cout << "OnQuote" << std::endl;
  {
    lock_guard lock(mutex);
    state.set_lua_state(l);
    state.on_quote(std::string(sec_class()), std::string(sec_code()));
  }
  std::cout << "OnQuote done" << std::endl;
}

LUACPP_STATIC_FUNCTION2(main, my_main)
LUACPP_STATIC_FUNCTION3(OnStop, OnStop, int)
LUACPP_STATIC_FUNCTION3(OnOrder, OnOrder, ::qlua::table::orders)
LUACPP_STATIC_FUNCTION4(OnQuote, OnQuote, const char*, const char*)
                        
extern "C" {
  LUALIB_API int luaopen_lualib_l2q_spread_bot(lua_State *L) {
    lua::state l(L);

    ::lua::function::main().register_in_lua(l, my_main);
    ::lua::function::OnStop().register_in_lua(l, OnStop);
    ::lua::function::OnOrder().register_in_lua(l, OnOrder);
    ::lua::function::OnQuote().register_in_lua(l, OnQuote);

    luaL_openlib(L, "lualib_l2q_spread_bot", ls_lib, 0);
    return 0;
  }
}

#define LUA_LIB
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
  #define LUA_BUILD_AS_DLL
#endif

#include <chrono>
#include <thread>

#include <qluacpp/qlua>

#include "model.hpp"
#include "gui.hpp"

static struct luaL_reg ls_lib[] = {
  { NULL, NULL }
};

void my_main(lua::state& l) {
  using namespace std::chrono_literals;
  qlua::api q(l);
  static model m(q, "QLUACPP_DEMO", 50);
  try {
    gui::instance().create("QluaCPP candles demo", &m);
  } catch (std::runtime_error e) {
    q.message((std::string("qluacpp candles demo failed to create gui window: ")
               + e.what()).c_str());
    return;
  }
  while (!gui::instance().done_) {
    std::this_thread::sleep_for(200ms);
  }
}

std::tuple<int> OnStop(const lua::state& l,
                       ::lua::entity<::lua::type_policy<int>> signal) {
  gui::instance().terminate();
  return std::make_tuple(int(1));
}

LUACPP_STATIC_FUNCTION2(main, my_main)
LUACPP_STATIC_FUNCTION3(OnStop, OnStop, int)
                        
extern "C" {
  LUALIB_API int luaopen_lualib_draw_candles(lua_State *L) {
    lua::state l(L);

    ::lua::function::main().register_in_lua(l, my_main);
    ::lua::function::OnStop().register_in_lua(l, OnStop);

    luaL_openlib(L, "lualib_draw_candles", ls_lib, 0);
    return 0;
  }
}

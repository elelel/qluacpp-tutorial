#define LUA_LIB
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
  #define LUA_BUILD_AS_DLL
#endif

#include <chrono>
#include <thread>

#include <qlua>

static struct luaL_reg ls_lib[] = {
  { NULL, NULL }
};

void my_main(lua::state& l) {
  using namespace std::chrono_literals;
  qlua::extended_api q(l);
  q.message("qluacpp tutorial: Starting main handler");
  for (int i = 0; i < 5; ++i) {
    q.message("qluacpp tutorial: Tick " + std::to_string(i));
    std::this_thread::sleep_for(1s);
  }
  q.message("qluacpp tutorial: Terminating main handler");
}

extern "C" {
  LUALIB_API int luaopen_lualib_qluacpp_tutorial(lua_State *L) {
    lua::state l(L);
    qlua::extended_api q(l);

    q.set_callback<qlua::callback::main>(my_main);

    luaL_openlib(L, "lualib_qluacpp_tutorial", ls_lib, 0);
    return 0;
  }
}

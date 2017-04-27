#define LUA_LIB
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
  #define LUA_BUILD_AS_DLL
#endif

#include <chrono>
#include <thread>

#include <qluacpp/qlua>

static struct luaL_reg ls_lib[] = {
  { NULL, NULL }
};

void my_main(lua::state& l) {
  using namespace std::chrono_literals;
  qlua::api q(l);
  q.message("qluacpp tutorial: Starting main handler");
  for (int i = 0; i < 5; ++i) {
    q.message(("qluacpp tutorial: Tick " + std::to_string(i)).c_str());
    std::this_thread::sleep_for(1s);
  }
  q.message("qluacpp tutorial: Terminating main handler");
}

LUACPP_STATIC_FUNCTION2(main, my_main)
                        
extern "C" {
  LUALIB_API int luaopen_lualib_basic_tutorial(lua_State *L) {
    lua::state l(L);

    ::lua::function::main().register_in_lua(l, my_main);

    luaL_openlib(L, "lualib_basic_tutorial", ls_lib, 0);
    return 0;
  }
}

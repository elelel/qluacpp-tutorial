#define LUA_LIB
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
  #define LUA_BUILD_AS_DLL
#endif

#include <chrono>
#include <thread>

#include <qlua>

#include "calendar.hpp"
#include "dividends_table.hpp"

static struct luaL_reg ls_lib[] = {
  { NULL, NULL }
};

void my_main(lua::state& l) {
  using namespace std::chrono_literals;
  qlua::extended_api q(l);

  calendar c;
  const std::string url{"http://smart-lab.ru/dividends"};
  if (!c.download(url)) {
    q.message("Can't download dividend calendar at " + url + "\n" + c.status_log());
  } else {
    q.message("Dividend calendar download report\n" + c.status_log());
    dividends_table t(c);
    t.show(q);
  }
}

extern "C" {
  LUALIB_API int luaopen_lualib_dividend_threat(lua_State *L) {
    lua::state l(L);
    qlua::extended_api q(l);

    q.set_callback<qlua::callback::main>(my_main);

    luaL_openlib(L, "lualib_dividend_threat", ls_lib, 0);
    return 0;
  }
}

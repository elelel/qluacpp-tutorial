#define LUA_LIB
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
  #define LUA_BUILD_AS_DLL
#endif

#include <chrono>
#include <memory>
#include <thread>

#include <qluacpp/qlua>

#include "model.hpp"
#include "gui.hpp"

static struct luaL_reg ls_lib[] = {
  { NULL, NULL }
};

static std::unique_ptr<model> m;

void my_main(lua::state& l) {
  using namespace std::chrono_literals;
  qlua::api q(l);
  static std::string sec_class{"TQBR"};
  static std::string sec_code{"SBER"};
  m = std::unique_ptr<model>(new model(q, sec_class, sec_code, q.constant<unsigned int>("INTERVAL_M1"), 50));
  try {
    gui::instance().data_unavailable_text() = {"Data is not available..."};
    gui::instance().create("QluaCPP RT candles demo - " + sec_class + ":" + sec_code, m.get());
  } catch (std::runtime_error e) {
    q.message((std::string("qluacpp rt candles demo failed to create gui window: ")
               + e.what()).c_str());
    return;
  }
  while (!gui::instance().done_) {
    std::this_thread::sleep_for(100ms);
  }
}

std::tuple<int> OnStop(const lua::state& l,
                       ::lua::entity<::lua::type_policy<int>> signal) {
  gui::instance().terminate();
  return std::make_tuple(int(1));
}

void qluacpp_candles_cb(const lua::state&l,
                        const ::lua::entity<::lua::type_policy<unsigned int>> idx) {
  m->update(idx());
}

LUACPP_STATIC_FUNCTION2(main, my_main)
LUACPP_STATIC_FUNCTION3(OnStop, OnStop, int)
LUACPP_STATIC_FUNCTION3(qluacpp_candles_cb, qluacpp_candles_cb, unsigned int)
                        
extern "C" {
  LUALIB_API int luaopen_lualib_draw_candles_rt(lua_State *L) {
    lua::state l(L);

    ::lua::function::main().register_in_lua(l, my_main);
    ::lua::function::OnStop().register_in_lua(l, OnStop);
    ::lua::function::qluacpp_candles_cb().register_in_lua(l, qluacpp_candles_cb);

    luaL_openlib(L, "lualib_draw_candles_rt", ls_lib, 0);
    return 0;
  }
}

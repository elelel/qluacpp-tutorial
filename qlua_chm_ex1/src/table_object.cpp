#define LUA_LIB
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
  #define LUA_BUILD_AS_DLL
#endif

#include <chrono>
#include <thread>

#include <qluacpp/qlua>

#include "quik_table_wrapper.hpp"
#include "ntime.hpp"

static struct luaL_reg ls_lib[] = {
  { NULL, NULL }
};

static bool stopped{false};

std::string format1(const int data) {
  char buf[256];
  snprintf(buf, 256, "0x%08X", data);
  return std::string(buf);
}

std::string format2(const int data) {
  char buf[256];
  snprintf(buf, 256, "%06d", data);
  return std::string(buf);
}

std::tuple<int> OnStop(const lua::state& l,
           ::lua::entity<::lua::type_policy<int>> signal) {
  stopped = true;
  return std::make_tuple(int(100));
}

void my_main(lua::state& l) {
  const std::vector<std::string> palochki{"-","\\", "|", "/"};
  qlua::api q(l);
  auto t = QTable(q);
  q.message(("table with id = " + std::to_string(t.t_id()) + " created").c_str(), 1);
  t.AddColumn("test1", q.constant<int>("QTABLE_INT_TYPE"), 10, format1);
  t.AddColumn("test2", q.constant<int>("QTABLE_INT_TYPE"), 10, format2);
  t.AddColumn("test3", q.constant<int>("QTABLE_CACHED_STRING_TYPE"), 50);
  t.AddColumn("test4", q.constant<int>("QTABLE_TIME_TYPE"), 50);
  t.AddColumn("test5", q.constant<int>("QTABLE_CACHED_STRING_TYPE"), 50);
  t.SetCaption("Test");
  t.Show();

  int i = 1;
  using namespace std::chrono_literals;
  using namespace std::chrono;
  while (!stopped) {
    if (t.IsClosed()) {
      t.Show();
    }
    t.SetCaption("QLUA TABLE TEST " + palochki[i % 4]);
    auto row = t.AddLine();
    t.SetValue<int>(row, "test1", i);
    t.SetValue<int>(row, "test2", i);

    q.SetCell(t.t_id(), row, 3, q.GetWindowCaption(t.t_id()));

    std::time_t date_tt = system_clock::to_time_t(system_clock::now());
    tm tm_info;
#ifdef _MSC_VER
    // Miscrosoft compilers break C++11 standard
    localtime_s(&tm_info, &date_tt);
#else
    localtime_s(&date_tt, &tm_info);
#endif
    
    char buf[256];
    snprintf(buf, 256, " (%02d:%02d:%02d)", tm_info.tm_hour, tm_info.tm_min, tm_info.tm_sec);
    q.SetCell(t.t_id(), row, 4,
              (NiceTime(tm_info) + buf).c_str(),
              tm_info.tm_hour*10000+tm_info.tm_min*100 + tm_info.tm_sec);
    q.SetCell(t.t_id(), row, 5, NiceTime(tm_info).c_str());
    std::this_thread::sleep_for(1s);
    i += 1;
  }
  q.message("finished");
}

LUACPP_STATIC_FUNCTION2(main, my_main)
LUACPP_STATIC_FUNCTION3(OnStop, OnStop, int)

extern "C" {
  LUALIB_API int luaopen_lualib_qlua_chm_ex1(lua_State *L) {
    lua::state l(L);

    ::lua::function::main().register_in_lua(l, my_main);
    ::lua::function::OnStop().register_in_lua(l, OnStop);

    luaL_openlib(L, "lualib_qlua_chm_ex1", ls_lib, 0);
    return 0;
  }
}

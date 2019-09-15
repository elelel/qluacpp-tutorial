#define LUA_LIB
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
  #define LUA_BUILD_AS_DLL
#endif

#include <chrono>
#include <thread>
#include <ctime>
#include <fstream>
#include <iostream>
#include <iomanip>

#include <qluacpp/qlua>

static struct luaL_reg ls_lib[] = {
  { NULL, NULL }
};

static bool run = true;
static std::ofstream file;
static std::unique_ptr<qlua::data_source> ds1;
static std::unique_ptr<qlua::data_source> ds2;

void remove_crlf(char c[]) {
  const auto len = strlen(c);
  while ((len > 0) && ((c[len-1] == '\n') || (c[len-1] == '\r')))
    c[len-1] = 0x00;
};

void log(const std::string& str) {
  using std::chrono::system_clock;
  std::time_t tt = system_clock::to_time_t(system_clock::now());
  char timec_written[256];
  ctime_s(timec_written, 256, &tt);
  remove_crlf(timec_written);
  file << timec_written << "\t" <<  str << '\n';
  std::cout << timec_written << "\t" <<  str << '\n';
}

void ds1_cb(const lua::state& l, const ::lua::entity<::lua::type_policy<int>> idx) {
  log("ds1_cb, index " + std::to_string(idx()));
}

void ds2_cb(const lua::state& l, const ::lua::entity<::lua::type_policy<int>> idx) {
  log("ds2_cb, index " + std::to_string(idx()));
}

void my_main(lua::state& l) {
  using namespace std::chrono_literals;
  file.open("qluacpp_datasource_log.txt");
  log("Started main");
  log("Calling callback 1 directly");
  l.getglobal("ds1_cb");
  if (l.isfunction(-1)) {
    lua_pushinteger(l.C_state(), 0);
    l.pcall(1, 0, 0);
  } else {
    log("Failed to call callback 1 directly");
    return;
  }
  log("Calling callback 2 directly");
  l.getglobal("ds2_cb");
  if (l.isfunction(-1)) {
    lua_pushinteger(l.C_state(), 0);
    l.pcall(1, 0, 0);
  } else {
    log("Failed to call callback 2 directly");
    return;
  }
  log("Creating datasources with callbacks ");
  qlua::api q(l);
  auto interval = q.constant<unsigned int>("INTERVAL_M1");
  auto ds1 = std::unique_ptr<qlua::data_source>
    (new ::qlua::data_source
     (q.CreateDataSource("INDX", "MOEX10", interval)));
  if (!ds1->SetUpdateCallback("ds1_cb")) {
    log("Failed to set update callback on datasource 1: false returned");
    return;
    }
  auto ds2 = std::unique_ptr<qlua::data_source>
    (new ::qlua::data_source
     (q.CreateDataSource("INDX", "MOEX", interval)));
  if (!ds2->SetUpdateCallback("ds2_cb")) {
    log("Failed to set update callback on datasource 2: false returned");
    return;
  }
  while (run) {
    std::this_thread::sleep_for(500ms);
  }
  log("Main finished");
}

std::tuple<int> OnStop(const lua::state& l,
                       ::lua::entity<::lua::type_policy<int>> signal) {
  log("OnStop qluacpp_datasource");
  run = false;
  file.flush();
  if (ds1) {
    ds1->Close();
    ds1 = nullptr;
  }
  if (ds2) {
    ds2->Close();
    ds2 = nullptr;
  }
  return std::make_tuple(int(1));
}

LUACPP_STATIC_FUNCTION2(main, my_main)
LUACPP_STATIC_FUNCTION3(OnStop, OnStop, int)
LUACPP_STATIC_FUNCTION3(ds1_cb, ds1_cb, int)
LUACPP_STATIC_FUNCTION3(ds2_cb, ds2_cb, int)

extern "C" {
  LUALIB_API int luaopen_lualib_datasource(lua_State *L) {
    lua::state l(L);

    ::lua::function::main().register_in_lua(l, my_main);
    ::lua::function::OnStop().register_in_lua(l, OnStop);
    ::lua::function::ds1_cb().register_in_lua(l, ds1_cb);
    ::lua::function::ds2_cb().register_in_lua(l, ds2_cb);

    std::cout << "::lua::function::ds1_cb() " << std::hex << &::lua::function::ds1_cb()
              << "\n"
              << "::lua::function::ds2_cb() " << std::hex << &::lua::function::ds2_cb()
              << "\n";
    
    luaL_openlib(L, "lualib_datasource", ls_lib, 0);
    return 0;
  }
}

#define LUA_LIB
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
#define LUA_BUILD_AS_DLL
#endif

#include <chrono>
#include <thread>

#include <qluacpp/qlua>

#include "trade_logger.hpp"

#include <iostream>

static struct luaL_reg ls_lib[] = {
  { NULL, NULL }
};

void my_main(lua::state& l) {
  using namespace std::chrono_literals;
  qlua::api q(l);
  // Get string with all classes
  auto classes = q.getClassesList();
  if (classes != nullptr) {
    std::string class_name;
    classes_record r_classes;
    char* ptr = (char*)classes;
    while (strlen(ptr) > 0) {
      if (*ptr == ',') {
        r_classes.codes.push_back(class_name);
        class_name = "";
      } else class_name += *ptr;
      ++ptr;
    }
    log_record rec;
    rec.rec_type = record_type::CLASSES;
    rec.classes = r_classes;
    rec.time = std::chrono::system_clock::now();
    trade_logger::instance().update(rec);

    // Get class info for each class, and securities
    for (const auto& code : r_classes.codes) {
      class_info_record r;
      try {
        q.getClassInfo(code.c_str(), [&r] (const auto& class_info) {
            r.name = class_info().name();
            r.code = class_info().code();
            r.npars = class_info().npars();
            r.nsecs = class_info().nsecs();
          });
        auto class_secs = q.getClassSecurities(code.c_str());
        char* ptr = (char*)class_secs;
        std::string sec_name;
        while (strlen(ptr) > 0) {
          if (*ptr == ',') {
            r.securities.push_back(sec_name);
            sec_name = "";
          } else sec_name += *ptr;
          ++ptr;
        }
        rec.rec_type = record_type::CLASS_INFO;
        rec.class_info = r;
        rec.time = std::chrono::system_clock::now();
        trade_logger::instance().update(rec);
      } catch (std::exception e) {
        q.message((std::string("log_all_trades: failed to run getClassInfo for ") +
                   code + ", exception: " + e.what()).c_str());
      }
    }
  } else {
    q.message("log_all_trades: getClassesList returned nullptr!");
  }
  while (true) {
    std::this_thread::sleep_for(1s);
  }
}

void OnAllTrade(const lua::state& l,
                ::lua::entity<::lua::type_policy<::qlua::table::all_trades_no_datetime>> data) {
  try {
    // Create log record in our format
    log_record rec;
    rec.rec_type = record_type::ALL_TRADE;
    all_trade_record r;
    // Copy data from callback
    r.class_code = data().class_code();
    r.sec_code = data().sec_code();
    r.price = data().price();
    r.value = data().value();
    r.qty = data().qty();

    // Create all_trades structure that contains time
    ::qlua::table::all_trades data_with_time(l, -1);
    try {
      const auto& d = data_with_time.datetime();
      std::cout << "Got OnAllTrade " << r.class_code << ":" << r.sec_code
                << ", server timestamp - " << d.hour << ":" << d.min << ":" << d.sec << "." << d.ms << std::endl;
    } catch (std::runtime_error e) {
      std::cout << "Error accessing all_trades with time structure - " << e.what() << std::endl;
    }
    // Request additional data on instrument from QLua
    qlua::api q(l);
    q.getSecurityInfo(r.class_code.c_str(), r.sec_code.c_str(),
                      [&r] (const auto& sec_info) {
                        r.name = sec_info().name();
                        return 1;  // How many stack items should be cleaned up (poped)
                      });
    rec.all_trade = r;
    // Record record creation time
    rec.time = std::chrono::system_clock::now();
    // Send the record to trade logger
    trade_logger::instance().update(rec);
  } catch (...) {
    // Ignore all errors, e.g. events from futures market
  }
}

std::tuple<int> OnStop(const lua::state& l,
                       ::lua::entity<::lua::type_policy<int>> signal) {
  trade_logger::instance().terminate();
  return std::make_tuple(int(1));
}

LUACPP_STATIC_FUNCTION2(main, my_main)
LUACPP_STATIC_FUNCTION3(OnAllTrade, OnAllTrade, ::qlua::table::all_trades_no_datetime)
LUACPP_STATIC_FUNCTION3(OnStop, OnStop, int)

extern "C" {
  LUALIB_API int luaopen_lualib_log_all_trades(lua_State *L) {
    lua::state l(L);

    ::lua::function::main().register_in_lua(l, my_main);
    ::lua::function::OnAllTrade().register_in_lua(l, OnAllTrade);
    ::lua::function::OnStop().register_in_lua(l, OnStop);

    luaL_openlib(L, "lualib_log_all_trades", ls_lib, 0);
    return 0;
  }
}

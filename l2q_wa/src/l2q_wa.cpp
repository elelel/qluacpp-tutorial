#define LUA_LIB
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
  #define LUA_BUILD_AS_DLL
#endif

#include <chrono>
#include <memory>
#include <thread>

#include <qluacpp/qlua>

#include "wa_table.hpp"

static struct luaL_reg ls_lib[] = {
  { NULL, NULL }
};

static size_t depth_count{3};
static bool done{false};
static std::string sec_class{"TQBR"};
static std::string sec_code{"SBER"};
static wa_table table;

void my_main(lua::state& l) {
  using namespace std::chrono_literals;
  qlua::api q(l);
  table.create(q, ("Average L2Q for " + sec_class + ":" + sec_code + ", depth " + std::to_string(depth_count)).c_str());
  if (q.Subscribe_Level_II_Quotes("TQBR", "SBER")) {
    while (!done) {
      std::this_thread::sleep_for(100ms);
    }
  } else {
    q.message(("Could not subscribe to " + sec_class + ":" + sec_code).c_str());
  }
}

void OnQuote(const lua::state& l,
             ::lua::entity<::lua::type_policy<const char*>> quote_sec_class,
             ::lua::entity<::lua::type_policy<const char*>> quote_sec_code) {
  if ((quote_sec_class() == sec_class) && (quote_sec_code() == sec_code)) {
    qlua::api q(l);
    q.getQuoteLevel2(quote_sec_class(), quote_sec_code(),
                     [&q] (const ::qlua::table::level2_quotes& quotes) {
                       auto bid = quotes.bid();
                       auto offer = quotes.offer();
                       double bid_quant{0.0};
                       double bid_wap{0.0};
                       double offer_quant{0.0};
                       double offer_wap{0.0};
                       if ((depth_count <= bid.size()) && (depth_count <= offer.size())) {
                         for (size_t i = 0; i < depth_count; ++i) {
                           auto& b = bid[bid.size() - 1 - i];
                           const double bq = atof(b.quantity.c_str());
                           bid_quant += bq;
                           bid_wap += atof(b.price.c_str()) * bq;
                           auto& o = offer[i];
                           const double oq = atof(o.quantity.c_str());
                           offer_quant += oq;
                           offer_wap += atof(o.price.c_str()) * oq;
                         }
                         bid_wap = bid_wap / bid_quant;
                         offer_wap = offer_wap / offer_quant;
                         table.update(q, bid_wap, bid_quant, offer_wap, offer_quant);
                         }
                         
                     });
  }
}

LUACPP_STATIC_FUNCTION2(main, my_main)
LUACPP_STATIC_FUNCTION4(OnQuote, OnQuote, const char*, const char*)
                        
extern "C" {
  LUALIB_API int luaopen_lualib_l2q_wa(lua_State *L) {
    lua::state l(L);

    ::lua::function::main().register_in_lua(l, my_main);
    ::lua::function::OnQuote().register_in_lua(l, OnQuote);

    luaL_openlib(L, "lualib_l2q_wa", ls_lib, 0);
    return 0;
  }
}

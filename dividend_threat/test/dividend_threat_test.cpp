#include "catch.hpp"

#include <ctime>
#include <iostream>

#include "calendar.hpp"

SCENARIO("Test calendar record") {
  GIVEN("Record info as strings") {
    std::string ticker{"LSNGP"};
    std::string date_str{"21.06.2017"};
    std::string div_return{"47.3"};
    WHEN("Calendar record is created") {
      calendar_record r(ticker, date_str, div_return);
      THEN("The ticker field should match") {
        REQUIRE(r.ticker() == ticker);
      }
      THEN("The date field should match") {
        auto actual_tm = r.date<std::tm>();
        REQUIRE(actual_tm.tm_year == 2017 - 1900);
        REQUIRE(actual_tm.tm_mon == 6 - 1);
        REQUIRE(actual_tm.tm_mday == 21);
      }
      THEN("Dividend return should match") {
        REQUIRE(r.div_return<const std::string&>() == "47.3");
        REQUIRE(r.div_return<double>() == double{47.3});
      }
      THEN("Time manipulation should be ok") {
        std::time_t tt = r.date<std::time_t>();
        auto tm = localtime(&tt);
        REQUIRE(tm->tm_year == r.date<std::tm>().tm_year);
        REQUIRE(tm->tm_mon == r.date<std::tm>().tm_mon);
        REQUIRE(tm->tm_mday == r.date<std::tm>().tm_mday);
      }
      THEN("Days left should be reasonable") {
        REQUIRE(r.days_left() < 10000);
      }
    }
  }
}

SCENARIO("Test dividends block extraction") {  
  GIVEN("Synthetic HTML page") {
    const std::string html = R"(
This should not be included 1
	<div class="shares_filter">
This should not be included 2
</div>
<table >
<tr> Header row
This should not be included 3
</tr>
<tr>
This should be included
</tr>
<tr>
</tr>
</table>
This should not be included 5
)";
    calendar c;
    WHEN("Dividends block is extracted") {
      auto extracted = c.extract_dividends_block(html);
      THEN("Only correct parts should be included") {
        REQUIRE(extracted.find("This should not be included 1") == std::string::npos);
        REQUIRE(extracted.find("This should not be included 2") == std::string::npos);
        REQUIRE(extracted.find("This should not be included 3") == std::string::npos);
        REQUIRE(extracted.find("This should not be included 4") == std::string::npos);
        REQUIRE(extracted.find("This should not be included 5") == std::string::npos);
        REQUIRE(extracted.find("This should be included") != std::string::npos);
      }
    }
       
  }
}

SCENARIO("Test record extraction") {
  GIVEN("Synthetic HTML record") {
    const std::string html = R"(
		<tr >
			<td><a href="href">Name<1/a></td>
			<td>TICKER1</td>
			<td>
									field2
							</td>
			<td>
									05.06.2017
							</td>
			<td>2016</td>
			<td>
									год
							<td><strong>123,45</strong> <span title="">П</span></td>
			<td>6789</td>
			<td><strong>12,3%</strong> <span title="Прогноз">П</span></td>
					</tr>
)";
    calendar c;
    WHEN("Extract record") {
      auto r = c.extract_record(html);
      THEN("Record fields should match") {
        REQUIRE(r.ticker() == "TICKER1");
      }
    }
  }
}

SCENARIO("Test page split") {
  GIVEN("Synthetic HTML page") {
    const std::string html = R"(
       HTML prefix
	<div class="shares_filter">
		<form method="GET">
		<table>
		<tr>
			<td>
 options
			</td>
 button
			</td>
		</tr>
		<tr>
			<td>
 checkbox
			</td>
		</tr>
		</table>
		</form>
	</div>

	</div>

		<table >
		<tr>
 table header
					</tr>	
		
		<tr >
			<td><a href="href">Name<1/a></td>
			<td>TICKER1</td>
			<td>
									&nbsp;
							</td>
			<td>
									&nbsp;
							</td>
			<td>2016</td>
			<td>
									год
							<td><strong>123,45</strong> <span title="">П</span></td>
			<td>6789</td>
			<td><strong>12,3%</strong> <span title="Прогноз">П</span></td>
					</tr>
								
		
		<tr >
			<td><a href="href" title="Name2">Name 2</a></td>
			<td>TICKER2</td>
			<td>
									&nbsp;
							</td>
			<td>
									02.01.2017 <span title="Прогноз">П</span>							</td>
			<td>2016</td>
			<td>
									год
							<td><strong>0.34</strong> <span title="Прогноз">П</span></td>
			<td>2,123</td>
			<td><strong>5,67%</strong> <span title="Прогноз">П</span></td>
			
					</tr>
              </table>

)";
    calendar c;
    auto div_block = c.extract_dividends_block(html);
    THEN("Extract") {
      auto rs = c.extract_records(div_block);
      REQUIRE(rs.size() == 1);
    }
  }
 
}


SCENARIO("Download and parse") {
  WHEN("Download calendar html") {
    calendar c;
    const std::string url{"http://smart-lab.ru/dividends"};
    c.download(url);
    REQUIRE(c.records().size() > 10);
  }
}

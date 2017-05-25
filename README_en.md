## Tutorial for QluaCpp library ##

[**QluaCpp**](https://github.com/elelel/qluacpp) is a library to write C++ plugins for **Quik** trading terminal

### Running the code ###
Clone the repository **recursing all submodules**. Then run CMake to generate project from a tutorial's root. The code is tested only with Microsoft compiler.

### Tutorials (Ru) ###

 - [Basic](basic_tutorial) tutorial. Shows how to set up the environment and write a very simple plugin that displays counting messages in Quik terminal:
 
 ![Message screenshot](basic_tutorial/doc/message_screenshot.png)
 
 - [Dividend threat](dividend_threat) tutorial. A simple plugin that downloads dividend close dates and expected returns from the Web, then displays a table. It highlights each instrument according to "threat" level, assuming the sooner dividend cutoff date is and the higher are expected returns - the more threat pressure the instrument experiences. Demonstrates how to use **qluacpp** to draw table windows in Quik terminal
 
 
 ![Dividend threat screenshot](dividend_threat/doc/table_screenshot.png)
 
- [Log all market trades](log_all_trades). Plugin that logs all market trades to a text file asynchronously in a separate thread:


```
Event at Thu May 25 03:59:49 2017, written at Thu May 25 03:59:49 2017. All classes. Class codes: EQOB, PSAU, PSSU, AUCT, AUCT_BND, SMAL, EQDB, EQQI, TQBR, TQOB, TQQI, TQDE, INDX, RTSIDX, FRGNIDX, TQIF, TQTF, REPORT, ALGO_ICEBERG, ALGO_VOLATIL, ALGO_GTD, TQTC, EQRP, PSRP, TQOD
Event at Thu May 25 03:59:49 2017, written at Thu May 25 03:59:49 2017. Class info. Name: А1-Облигации, Code: EQOB, Number of parameters: 64, Number of securities: 1239
Event at Thu May 25 03:59:49 2017, written at Thu May 25 03:59:49 2017. Class info. Name: РПС: ММВБ ФБ: Первичное размещение (облигации), Code: PSAU, Number of parameters: 64, Number of securities: 23
Event at Thu May 25 03:59:49 2017, written at Thu May 25 03:59:49 2017. Class info. Name: РПС: ММВБ ФБ: Первичное размещение (акции), Code: PSSU, Number of parameters: 64, Number of securities: 0
...
Event at Thu May 25 04:02:39 2017, written at Thu May 25 04:02:49 2017. All trades. Name: Таганрогский комб.завод ОАО ао, Code: TGKO, Price: 0.185, Value: 1.85, Qty: 1
Event at Thu May 25 04:02:40 2017, written at Thu May 25 04:02:49 2017. All trades. Name: Таганрогский комб.завод ОАО ао, Code: TGKO, Price: 0.185, Value: 1.85, Qty: 1
Event at Thu May 25 04:02:40 2017, written at Thu May 25 04:02:49 2017. All trades. Name: Таганрогский комб.завод ОАО ао, Code: TGKO, Price: 0.185, Value: 1.85, Qty: 1
Event at Thu May 25 04:02:41 2017, written at Thu May 25 04:02:49 2017. All trades. Name: Таганрогский комб.завод ОАО ао, Code: TGKO, Price: 0.185, Value: 1.85, Qty: 1
Event at Thu May 25 04:02:41 2017, written at Thu May 25 04:02:49 2017. All trades. Name: Таганрогский комб.завод ОАО ао, Code: TGKO, Price: 0.185, Value: 1.85, Qty: 1
Event at Thu May 25 04:02:42 2017, written at Thu May 25 04:02:49 2017. All trades. Name: Таганрогский комб.завод ОАО ао, Code: TGKO, Price: 0.185, Value: 1.85, Qty: 1
Event at Thu May 25 04:02:43 2017, written at Thu May 25 04:02:49 2017. All trades. Name: Таганрогский комб.завод ОАО ао, Code: TGKO, Price: 0.185, Value: 1.85, Qty: 1
Event at Thu May 25 04:02:43 2017, written at Thu May 25 04:02:49 2017. All trades. Name: Медиахолдинг (ПАО) ао, Code: ODVA, Price: 0.192, Value: 192, Qty: 1
Event at Thu May 25 04:02:43 2017, written at Thu May 25 04:02:49 2017. All trades. Name: Таганрогский комб.завод ОАО ао, Code: TGKO, Price: 0.185, Value: 1.85, Qty: 1
Event at Thu May 25 04:02:43 2017, written at Thu May 25 04:02:49 2017. All trades. Name: Таганрогский комб.завод ОАО ао, Code: TGKO, Price: 0.185, Value: 1.85, Qty: 1
Event at Thu May 25 04:02:44 2017, written at Thu May 25 04:02:49 2017. All trades. Name: Таганрогский комб.завод ОАО ао, Code: TGKO, Price: 0.185, Value: 1.85, Qty: 1
```

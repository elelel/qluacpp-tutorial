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
Event time	Time written	SecCode	Price	Value	Qty
Tue Apr 18 03:35:46 2017	Tue Apr 18 03:35:46 2017	GAZP	123.01	246020	200
Tue Apr 18 03:35:46 2017	Tue Apr 18 03:35:46 2017	GAZP	123.01	139001	113
Tue Apr 18 03:35:46 2017	Tue Apr 18 03:35:46 2017	GAZP	123	3690	3
Tue Apr 18 03:35:46 2017	Tue Apr 18 03:35:46 2017	ALRS	88.11	8811	1
Tue Apr 18 03:35:46 2017	Tue Apr 18 03:35:46 2017	GAZP	123	67650	55
```

## Tutorial for QluaCpp library ##

[**QluaCpp**](https://github.com/elelel/qluacpp) is a library to write C++ plugins for **Quik** trading terminal

### Running the code ###
Clone the repository **recursing all submodules**. Then run CMake to generate project from a tutorial's root. The code is tested only with Microsoft compiler.

### Tutorials (Ru) ###

 - [Basic](basic) tutorial. Shows how to set up the environment and write a very simple plugin that displays counting messages in Quik terminal:
 
 ![Message screenshot](basic/doc/message_screenshot.png)
 
 - [Dividend threat](dividend_threat) tutorial. A simple plugin that downloads dividend close dates and expected returns from the Web, then displays a table, highlighting each instrument according to "threat" level, assuming the sooner dividend cutoff date is and the higher are expected returns - the more threat pressure the instrument experiences. Demonstrates how to use **qluacpp** to draw table windows in Quik terminal
 
 ![Dividend threat screenshot](dividend_threat/doc/table_screenshot.png)
 


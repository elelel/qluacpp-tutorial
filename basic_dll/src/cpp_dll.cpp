#include "cpp_dll.hpp"

#include "windows.h"

void my_class_in_dll::show_messagebox() {
  LPCTSTR text = "Hello from external C++ dll";
  LPCTSTR caption = "Message";
  MessageBoxA(NULL, text, caption, MB_OK);
}

4. Создание CMake проекта

4.1. В директории code создаем файл *CMakeLists.txt*

4.2. В *CMakeLists.txt* указываем базовую конфигурацию CMake

```
cmake_minimum_required(VERSION 3.4.0)
project(qluacpp_tutorial)
```

4.3. Создаем переменные с путями к используемым в проекте библиотекам и их зависимостях:

```
set(TOP_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(QLUACPP "${TOP_DIR}/contrib/qluacpp")
set(LUACPP "${TOP_DIR}/contrib/qluacpp/contrib/luacpp")
set(LUA "${TOP_DIR}/contrib/lua")
```

4.4. Указываем для qluacpp через переменную QLUACPP_LUA_LIBRARY, путь к скаченному lib-файлу библиотеки Lua:

```
set(QLUACPP_LUA_LIBRARY "${LUA}/lua5.1.lib")
```

4.5. Указываем путь к загаловкам скаченной библиотеки Lua для qluacpp:

```
set(QLUACPP_LUA_INCLUDE_DIR "${LUA}/include")
```

4.6. Перечисляем директории, которые должны быть доступны для директивы #include препроцессора. Они включают и include-директории зависимостей используемых проектом библиотек:

```
include_directories(
  ${QLUACPP}/include
  ${LUACPP}/include
  ${QLUACPP_LUA_INCLUDE_DIR}
)
```

4.7. Подключаем компиляцию библиотеки qluacpp:
```
if (NOT TARGET qluacpp)
  add_subdirectory(${QLUACPP} qluacpp)
endif()
```

4.8. Перечисляем исходные файлы нашего плагина:

```
set(SOURCES
  src/qluacpp_tutorial.cpp
)
```

4.9. Указываем, что мы хотим создать DLL:

```
add_library(lualib_qluacpp_tutorial SHARED ${SOURCES})
```

4.10. Указываем конфигурацию линковки:

```
target_link_libraries(lualib_qluacpp_tutorial qluacpp)
```

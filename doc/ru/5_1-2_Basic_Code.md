5. Пишем код плагина

5.1. Создаем файл *qluacpp_tutorial.cpp* в директории *code/src*

5.2. Пишем "основу" плагина в *qluacpp_tutorial.cpp*

5.2.1 Декларируем через директивы препроцессора, что мы создаем "библиотеку" Lua:

```c++
#define LUA_LIB
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
  #define LUA_BUILD_AS_DLL
#endif
```

5.2.2. После этого подключаем библиотеку qluacpp:

```c++
#include <qlua>
```

5.2.3. Создаем глобальную структуру для интерфейса наших "библиотечных" функций для Lua

```c++
static struct luaL_reg ls_lib[] = {
  { NULL, NULL }
};
```

5.2.4. Создаем функцию с C ABI, которая будет инициализировать "библиотеку"

```c++
extern "C" {
  LUALIB_API int luaopen_lualib_qluacpp_tutorial(lua_State *L) {
    // Here will be code...
    return 0;
  }
}
```

5.2.5. В теле этой функции:

5.2.5.1. Создаем объект интерфейса Lua с C++:

```c++
    lua::state l(L);
```

5.2.5.2. Создаем объект интерфейса Quik Lua:

```c++
    qlua::extended_api q(l);
```

Библиотека qluacpp предлагает два объекта API для взаимодействия с Quik Lua: api и extended_api. Объект api является максимально близким к родному интерфейсу QLUA от Arqa, вплоть до повторения опечаток в названиях API функций, несоответствий и проч. Объект extended_api является надмножеством относительно объекта api, содержа также дополнительные функции для удобства программирования в C++, консистеность терминологии и др.

5.2.5.3. Подключаем нашу "библиотеку" к Lua:

```c++
luaL_openlib(L, "lualib_qluacpp_tutorial", ls_lib, 0);
```


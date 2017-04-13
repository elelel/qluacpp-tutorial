5.3. Используем некоторые функции библиотеки qlua

5.3.1. Пишем обработчик функции обратного вызова QLUA **main**.

Обработчик в течение первых пяти секунд после запуска плагина каждую секунду будет выводить сообщение с счетчиком в терминале Quik, после чего выведет сообщение о завершении работы

5.3.1.1. Подключаем заголовки stdlib c++

```c++
#include <chrono>
#include <thread>
```

5.3.1.2. Пишем callback-функцию

```c++
void my_main(lua::state& l) {
  using namespace std::chrono_literals;
  qlua::extended_api q(l);
  q.message("qluacpp tutorial: Starting main handler");
  for (int i = 0; i < 5; ++i) {
    q.message("qluacpp tutorial: Tick " + std::to_string(i));
    std::this_thread::sleep_for(1s);
  }
  q.message("qluacpp tutorial: Terminating main handler");
}
```

Функция принимает в качестве аргумента стейт lua (c++ обертка luacpp вокруг C библиотеки Lua). Используя его, создается объект extended_api. При помощи этого объекта происходит обращение к стандартной функции QLUA message, которая выводит сообщения в терминале. По завершении работы функции my_main, работа плагина завершается, в соответствии с правилами lifetime QLUA.

5.3.1.3. Регистрируем callback

В функции luaopen_qluacpp_tutorial добавляем регистрацию нашего callback:

```c++
q.set_callback<qlua::callback::main>(my_main);
```


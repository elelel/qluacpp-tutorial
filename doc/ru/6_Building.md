6. Создание директории сборки проекта

6.1. В меню Start Windows ищем Command Prompt for VS2017. При выборе следует помнить, что мы компилируем 32-битный проект. Поэтому, если компиляция происходит на 64-битном Windows, следует выбирать x64_x86 Cross Tools Command Prompt. 

6.2. В любом удобном месте, не внутри директории с репозитарием проекта, создаем директории, в которые будет сгенерирован при помощи CMake проект под нужную нам среду.

6.2.1. Проект (solution)  IDE Visual Studio 

Cоздаем директорию *qluacpp_tutorial_vs*. Заходим в нее и выполняем:
```
"c:\Program Files\CMake\bin\cmake.exe" -G "Visual Studio 15 2017" c:\path\qluacpp_tutorial\code
```
где последний аругмент - путь к коду проекта, а опция -G указывает, под какую среду генерировать проект.

Описание дальнейшей работы с Visual Studio не является предметом данного документа.

6.2.2. Компиляция библиотеки при помощи NMake

NMake - мейкер от Microsoft, который поставляется в составе Visual Studio, Windows Developer Tools и др.

Создаем директорию *qluacpp_tutorial_nmake*

Заходим в нее и выполняем:
```
"c:\Program Files\CMake\bin\cmake.exe" -G "NMake Makefiles" c:\path\qluacpp_tutorial\code
```

Далее запускаем:
```
NMake
```

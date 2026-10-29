# zhiros-module-example
Создание разных графических модулей для ZhirOS: [https://github.com/ilizavr/zhiros-recode](https://github.com/ilizavr/zhiros-kernel)

Этот репозиторий рассчитан исключительно на графические модули для ZhirOS

вскоре в релиз будут выложены файлы.mod - Их перетащите в папку ramdisk

Для компиляции window manager:
1) Перетащите WM_types.h в корень проекта
2) Перетащите window_manager.c, terminal.c в mod.examples
3) Всякие игры перетащите в mod.games
4) Перетащите tmpfs в mod.drivers

5) Далее зайдите в build32.sh и среди build_module вставте строку и измените данные на такие, какие у вас:
build_module mod.examples/terminal.c ramdisk/term.mod
		 (папка где файл и сам файл) (в ramdisk название_файла.mod)


6) Добавляем в конце init.h, но перед start_shell() строчку:
load_mod(ramdisk_letter,"./window_manager.mod");
(каждый новый модуль добавлять после window_manager)

## API WM для модулей:
### Взаимодействие с окном
```c
_window_create(const char *name) -> struct
_window_render(Window *window) -> None
_compositor_present() -> None 
```

### Рисование окна и его содержимого
```c
_clearframe_win(Window *window, u32 color) -> None
_draw_border_window(Window *window, u32 color) -> None
_draw_string(Window *window, const char *str, u32 x, u32 heght, u32 color) -> None
_window_fill_rect(Window *window, u32 x, u32 y, u32 width, u32 height, u32 color) -> None
_window_is_active(Window *window) -> bool #проверка на то, активно ли окно в данный момент, или нет.
```


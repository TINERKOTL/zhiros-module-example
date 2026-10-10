# zhiros-module-example
Создание разных графических модулей для ZhirOS: [https://github.com/ilizavr/zhiros-recode](https://github.com/ilizavr/zhiros-kernel)

Этот репозиторий рассчитан исключительно на графические модули для ZhirOS

Для window manager:
1) Добавьте файл в папку с zhirOS-kernel

2) Добавляем в конце init.h строчку:
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
```


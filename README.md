# zhiros-module-example
Создание разных графических модулей для ZhirOS: [https://github.com/ilizavr/zhiros-recode](https://github.com/ilizavr/zhiros-kernel)

Этот репозиторий рассчитан исключительно на графические модули для ZhirOS

Для window manager:
1) Добавьте файл в папку с zhirOS-kernel

2) Добавляем в конце init.h строчку:
load_mod(ramdisk_letter,"./window_manager.mod");

3) Добавляем в build32.sh после строчки "#compile modules": 
gcc $MODULE_FLAGS -c window_manager.c -o build/module.o
ld -m elf_i386 -T module.ld build/module.o -o build/module.elf
objcopy --set-section-flags .bss=alloc,load,contents -O binary build/module.elf ramdisk/window_manager.mod

## API WM для модулей:
### Взаимодействие с окном
```c
_window_create(const char *name) -> struct
_window_render(Window *window) -> None
_compositor_present(u32 (*mouse_now)[6]) -> None //mouse_now это двухмерный массив, в котором хранится курсор с помощью 0 и 1, где 0 - none, 1 - цвет
```

### Рисование окна и его содержимого
```c
_clearframe_win(Window *window, u32 color) -> None
_draw_border_window(Window *window, u32 color) -> None
_draw_string(Window *window, const char *str, u32 x, u32 heght, u32 color) -> None
_window_fill_rect(Window *window, u32 x, u32 y, u32 width, u32 height, u32 color) -> None
```


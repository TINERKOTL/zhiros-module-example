/*
 * Copyright (c) 2026 ilizavr & yellowhat
 * SPDX-License-Identifier: MIT
 */

// by TINERKOTL

/* 
Этот файл является шаблоном приложения.

Если вы придумаете функции, которые можно будет использовать в других приложениях, пишите в ТГ (ссылка в github в описании) -
и я добавлю эту функцию в основной WM
*/
#include "WM_types.h"

Window* (*window_create)(char *name);
void (*window_render)(Window *window);
void (*clearframe_win)(Window *window, u32 color);
void (*window_fill_rect)(Window *window, u32 x, u32 y, u32 width, u32 height, u32 color);
void (*draw_string)(Window *window, const char *str, u32 x, u32 heght, u32 color);
void (*draw_border_window)(Window *window, u32 color);
void (*compositor_present)();
bool (*window_is_active)(Window *window);

static void your_name_func() {
    bool *key_state_matrix = get_key_state_matrix(); //если необходимо чтение клавиатуры

    Window *window = window_create('name'); //создание окна

    window_render(window); //его рендер

    while(1) {
        /*
            Функции которые будут использованы в приложении (пишите сами или используйте готовые функции из основного window_manager)
        */
        compositor_present();
        yield();
    }
}

INIT void init(void* _resolve_function(char* name))
{
    api_init(_resolve_function);
    window_create = _resolve_function("_window_create");
    window_render = _resolve_function("_window_render");
    window_fill_rect = _resolve_function("_window_fill_rect");
    draw_string = _resolve_function("_draw_string");
    compositor_present = _resolve_function("_compositor_present");
    window_is_active = _resolve_function("_window_is_active");
    
    create_task("terminal", your_name_func); //создание потока
}
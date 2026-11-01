/*
 * Copyright (c) 2026 ilizavr & TINERKOTL & yellowhat
 * SPDX-License-Identifier: MIT
 */

/* 
Этот файл является шаблоном приложения.

Если вы придумаете функции, которые можно будет использовать в других приложениях, пишите в ТГ (ссылка в github в описании) -
и я добавлю эту функцию в основной WM
*/
#include "WM_types.h"
#include "../lib/zhirapi.h"

Window* (*window_create)(char *name);
void (*window_render)(Window *window);
void (*clearframe_win)(Window *window, u32 color);
void (*window_fill_rect)(Window *window, u32 x, u32 y, u32 width, u32 height, u32 color);
void (*draw_string)(Window *window, const char *str, u32 x, u32 heght, u32 color);
void (*draw_border_window)(Window *window, u32 color);
void (*compositor_present)();
bool (*window_is_active)(Window *window);

struct task *window_task;

struct Window *window;

static void resize_text_editor(Window *window, s32 old_width, s32 old_height) {
    if (!window || !window->buffer || window->width < 16 || window->height < 36)
        return;

    // Функция для отрисовки внутрь окна при изменении его размера
    // Если у вас постоянный размер, можете оставить его так
}

static void your_name_func() {
    bool *key_state_matrix = get_key_state_matrix(); //если необходимо чтение клавиатуры

    Window *window = window_create("name"); //создание окна
    if (!window)
        return;
    window->window_on_changed = 0; //1 - изменять размер окна запрещено, 0 - можно
    window->task = window_task;
    window->on_resize = resize_text_editor;

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
    
    window_task = create_task("you_name_task", your_name_func); //создание потока
}
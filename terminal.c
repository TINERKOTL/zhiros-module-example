/*
 * Copyright (c) 2026 ilizavr & TINERKOTL & yellowhat
 * SPDX-License-Identifier: MIT
 */

#include "../lib/zhirapi.h"
#include "WM_types.h"

Window* (*window_create)(char *name);
void (*window_render)(Window *window);
void (*clearframe_win)(Window *window, u32 color);
void (*window_fill_rect)(Window *window, u32 x, u32 y, u32 width, u32 height, u32 color);
void (*draw_string)(Window *window, const char *str, u32 x, u32 y, u32 color);
void (*draw_char_bg)(Window *window, char chr, u32 x, u32 y, u32 color, u32 bgcolor);
void (*draw_border_window)(Window *window, u32 color);
void (*compositor_present)();
bool (*window_is_active)(Window *window);
void (*desktop_fill_rect)(s32 x, s32 y, s32 width, s32 height, u32 color);
void (*desktop_draw_string)(const char *text, u32 x, u32 y, u32 color);
bool (*register_icon)(u32, u32, u32, DesktopIconDraw);
void (*window_resize)(Window *window, s32 new_width, s32 new_height);


static Window *terminal_window=0;
struct task *terminal_task=0;

static s32 curx=0, cury=20;
volatile u32 fb_addr=0,fb_size=0,fb_size_withoutlastline=0,fb_lastline_addr=0,fb_onelinesize=0,fb_secondline_addr=0;
volatile u32 screen_height = 0;

static void clearframe_hook() {
    memset((void*)fb_addr, 0, fb_size);
    curx = 0;
    cury = 20;
}

static void terminal_on_resize(Window *window, s32 old_width, s32 old_height) {
    if (!window || !window->buffer || window->width < 8 || window->height < 36)
        return;

    screen_height = window->height;

    u32 content_height = (u32)(screen_height - 20);
    u32 scroll_height = content_height > 16 ? content_height - 16 : 0;
    u32 line_height = content_height < 16 ? content_height : 16;

    fb_addr = (i_ptr)(window->buffer + window->width * 20);
    fb_size = (i_ptr)(content_height * window->width * sizeof(u32));
    fb_size_withoutlastline = (i_ptr)(scroll_height * window->width * sizeof(u32));
    fb_lastline_addr = fb_addr + fb_size_withoutlastline;
    fb_onelinesize = (i_ptr)(line_height * window->width * sizeof(u32));
    fb_secondline_addr = fb_addr + fb_onelinesize;
}

void roll_screen()
{
    memcpy((char*)fb_addr,(char*)fb_secondline_addr,fb_size_withoutlastline);
    memset((char*)fb_lastline_addr,0,fb_onelinesize);
}

static void putchar_color_hook(char chr, u32 color)
{
    if(window_is_active(terminal_window)){
        if(chr=='\n') {
            putchar_color_hook(' ',color);
            curx = 0;
            cury+=16;
            if((cury+16)>screen_height) {
                roll_screen();
                cury-=16;
            }
            return;
        }
        if(curx > (terminal_window->width-8)) {
            curx = 0;
            cury+=16;
            if((cury+16)>screen_height) {
                roll_screen();
                cury-=16;
            }
        }
        if(chr == '\b')
        {
            curx -= 8;
            if(curx<0)
            {
                curx = (terminal_window->width/8-1)*8;
                cury -= 16;
            }
            return;
        }
        draw_char_bg(terminal_window,chr,curx,cury,color,0);
        curx+=8;
    }
}

// void draw_terminal_icon(u32 x, u32 y, u32 size) {
//     desktop_fill_rect(x, y, size, size, 0x00004D78);
//     desktop_fill_rect(x + 3, y + 3, size - 6, size - 6, 0x00000000);
//     desktop_fill_rect(x + 9, y + 12, 5, 2, 0x00FFFFFF);
//     desktop_fill_rect(x + 15, y + 17, 12, 2, 0x00FFFFFF);
//     desktop_draw_string("Terminal", x - 4, y + size + 4, 0x00FFFFFF);
// }

static void terminal_thread() {
    terminal_window = window_create("terminal");
    if (!terminal_window) return;
    terminal_window->task = terminal_task;
    terminal_window->on_resize = terminal_on_resize;

    window_render(terminal_window);

    screen_height = terminal_window->height;
    fb_addr = (i_ptr)(terminal_window->buffer+terminal_window->width*20);
    fb_size = (i_ptr)(terminal_window->buffer_size*4-terminal_window->width*80);
    fb_size_withoutlastline = (i_ptr)(terminal_window->width*(screen_height-16)*4);
    fb_lastline_addr = fb_addr+fb_size_withoutlastline;
    fb_onelinesize = terminal_window->width*16*4;
    fb_secondline_addr = fb_addr+fb_onelinesize;

    replace_function("_putchar_color",putchar_color_hook);
    replace_function("_clearframe",clearframe_hook);

    clearframe_hook();

    while(1) {
        screen_height = terminal_window->height;
        fb_addr = (i_ptr)(terminal_window->buffer+terminal_window->width*20);
        fb_size = (i_ptr)(terminal_window->buffer_size*4-terminal_window->width*80);
        fb_size_withoutlastline = terminal_window->width * (screen_height - 36) * 4;
        fb_lastline_addr = fb_addr + fb_size_withoutlastline;   
        fb_onelinesize = terminal_window->width*16*4;
        fb_secondline_addr = fb_addr+fb_onelinesize;

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
    draw_char_bg = _resolve_function("_draw_char_bg");
    compositor_present = _resolve_function("_compositor_present");
    window_is_active = _resolve_function("_window_is_active");
    desktop_fill_rect = _resolve_function("_desktop_fill_rect");
    desktop_draw_string = _resolve_function("_desktop_draw_string");
    window_resize = _resolve_function("_window_resize");

    terminal_task = create_task("terminal", terminal_thread); //создание потока
}

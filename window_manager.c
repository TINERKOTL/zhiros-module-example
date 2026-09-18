/*
 * Copyright (c) 2026 ilizavr & yellowhat
 * SPDX-License-Identifier: MIT
 */


// typedef unsigned long uintptr_t;

#include "lib/zhirtypes.h"
#include "kernel32/shell/font8x16.h"
#include "lib/string.h"

u32 WINDOW_HEADER_Y = 20;

u32 WINDOW_X = 200;
u32 WINDOW_Y = 150;

u32 (*getchar)();
void *(*alloc)(u32 size);
void *(*free)(void *ptr);
void (*printf)(char* fmt, ...);
bool *(*get_key_state_matrix)();

bool close_window = false;

u32 window_width = 400;
u32 window_height = 300;
u32 *window_buffer = 0;

struct fb_info* (*fbcon_stop)();

struct fb_info* fb;

u32 init_window_buffer(u32 x, u32 y){
    window_height = y;
    window_width = x;

    window_buffer = alloc(window_width*window_height*sizeof(u32));
    return 1;
}

void clearframe_win(u32 color) {
    u32 count = window_height*window_width;

    for (int i = 0; i < count; i++){
        window_buffer[i] = color;
    }
}

void put_pixel(u32 x, u32 y, u32 color) {
    volatile u32 *pixel_addr = (volatile u32*)(fb->fb_addr + y * fb->screen_pitch + x * fb->bpp);

    *(volatile u32*)pixel_addr = color;
}

void put_pixel_window(u32 x, u32 y, u32 color)
{
    if (x >= window_width || y >= window_height)
        return;

    window_buffer[y * window_width + x] = color;
}

void draw_line_hor_window(u32 startx, u32 endx, u32 y, u32 color) {
    for (u32 x = startx; x < endx; x++)
        put_pixel_window(x, y, color);
}

void draw_line_hor(u32 startx, u32 endx, u32 y, u32 color) {
    for (u32 x = startx; x < endx; x++)
        put_pixel(x, y, color);
}

void draw_line_ver(u32 starty, u32 endy, u32 x, u32 color) {
    for (u32 y = endy; y < starty; y++)
        put_pixel(x, y, color);
}

void clearframe() {
    memset((u32*)fb->fb_addr, 0, fb->screen_height*fb->screen_pitch);
}

void draw_char(u8 code, u32 x, u32 y, u32 color)
{
    for (u32 row = 0; row < 16; row++) {
        for (u32 col = 0; col < 8; col++) {
            if (font8x16[16*code + row] & (0x80 >> col))
                put_pixel(x + col, y + row, color);
        }
    }
}

void draw_string(const char *str, u32 x, u32 y, u32 color)
{
    while (*str) {
        draw_char((u8)*str, x, y, color);
        x += 8;
        str++;
    }
}

void draw_border_window(u32 color) {
    for (u32 x = 0; x < window_width; x++) {
        put_pixel_window(x, 0, color);
        put_pixel_window(x, window_height - 1, color);
    }

    for (u32 y = 0; y < window_height; y++) {
        put_pixel_window(0, y, color);
        put_pixel_window(window_width - 1, y, color);
    }
}

void window_fill_rect(u32 x, u32 y, u32 width, u32 height, u32 color)
{
    for (u32 py = y; py < y + height; py++) {
        for (u32 px = x; px < x + width; px++) {
            put_pixel_window(px, py, color);
        }
    }
}

void present_window(u32 screen_x, u32 screen_y) {
    for (u32 y = 0; y < window_height; y++) {
        for (u32 x = 0; x < window_width; x++) {
            u32 dst_x = screen_x + x;
            u32 dst_y = screen_y + y;

            if (dst_x >= fb->screen_width)
                continue;

            if (dst_y >= fb->screen_height)
                continue;

            u32 color =
                window_buffer[
                    y * window_width + x
                ];

            put_pixel(
                dst_x,
                dst_y,
                color
            );
        }
    }
}

void draw_window_contents() {
    clearframe_win(0xAAAAAA);
    
    window_fill_rect(0, 0, window_width-1, 20, 0xFFFFFF);

    window_fill_rect(20, 45, 120, 80, 0xFF0000);

    draw_border_window(0x00FFFFFF);
}

void graphic_init() {
    clearframe();

    WINDOW_X = (fb->screen_width - window_width) / 2;
    WINDOW_Y = (fb->screen_height - window_height) / 2;

    draw_window_contents();
    present_window(WINDOW_X, WINDOW_Y);

    draw_string("hello", WINDOW_X + 5, WINDOW_Y + 2, 0x000000);

}

INIT void init(void* _resolve_function(char* name))
{
    printf = _resolve_function("_printf");
    getchar = _resolve_function("_getchar");
    fbcon_stop = _resolve_function("_fbcon_stop");
    alloc = _resolve_function("_alloc");

    get_key_state_matrix = _resolve_function("_get_key_state_matrix");
    bool *key_state_matrix = get_key_state_matrix();

    u8 wcode = ascii_to_keycode['w'];
    u8 dcode = ascii_to_keycode['d'];

    fb = fbcon_stop();

    init_window_buffer(400, 300);

    clearframe();

    graphic_init();

    while(1){
        if (key_state_matrix[wcode]){
            while(close_window != true){
                if (key_state_matrix[dcode])
                    close_window = true;
                graphic_init();
                for (u32 i = 0; i < 25000000; i++);
            }
        }
        clearframe();
        close_window = false;
    }
}

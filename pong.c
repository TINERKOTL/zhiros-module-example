/*
 * Copyright (c) 2026 ilizavr & yellowhat
 * SPDX-License-Identifier: MIT
 */

// by TINERKOTL
#include "lib/zhirtypes.h"
#include "WM_types.h"
#include "lib/string.h"

u32 (*getchar)();
void (*printf)(char* fmt, ...);
bool *(*get_key_state_matrix)();
void (*render_window)(Window *window);
void (*sleep)(u32 ms);
void (*compositor_present)();

Window* (*window_create)(char *name);

// Window* window;

static void put_pixel_window(Window *window, u32 x, u32 y, u32 color)
{
    if (x >= window->width || y >= window->height)
        return;

    window->buffer[y * window->width + x] = color;
}

void draw_line(Window *window, u32 startx, u32 endx, u32 y, u32 color) {
    for (u32 x = startx; x < endx; x++)
        put_pixel_window(window, x, y, color);
}

static void clearframe_win(Window *window, u32 color) {
    if (!window || !window->buffer)
        return;

    for (u32 x = 0; x < window->width; x++)
        window->buffer[x] = color;

    for (u32 y = 1; y < window->height; y++) {
        memcpy(&window->buffer[y * window->width], window->buffer, window->width * sizeof(u32));
    }
}

void draw_cube(Window *window, u32 startx, u32 endx, u32 starty, u32 endy, u32 color) {
    for (u32 x = startx; x < endx; x++)
        for (u32 y = starty; y < endy; y++)
            put_pixel_window(window, x, y, color);
}

bool collision(u32 cube_start, u32 cube_end, u32 platform_start, u32 platform_end) {
    return cube_end >= platform_start && cube_start <= platform_end;
}

// u32 x = 100;

INIT void init(void* _resolve_function(char* name))
{
    getchar = _resolve_function("_getchar");
    printf = _resolve_function("_printf");
    window_create = _resolve_function("_window_create");
    get_key_state_matrix = _resolve_function("_get_key_state_matrix");
    sleep = _resolve_function("_sleep_ms");
    compositor_present = _resolve_function("_compositor_present");
    render_window = _resolve_function("_window_render");

    bool *key_state_matrix = get_key_state_matrix();

    u8 wcode_a = ascii_to_keycode['a'];
    u8 wcode_d = ascii_to_keycode['d'];

    u8 wcode_j = ascii_to_keycode['j'];
    u8 wcode_l = ascii_to_keycode['l'];

    Window *window = window_create("Pong Game");

    render_window(window);

    u32 x = window->width / 2 - 50, x_2 = window->width / 2 - 50, end_x = window->width / 2 + 50, end_x_2 = window->width / 2 + 50;
    
    u32 cube_x = 100, cube_y = 150, cube_size = 7;
    u32 top_y = 50, bottom_y = window->height-33;
    u32 platform_center, cube_center;
    bool cube_direction_y = 0; // 1 - вверх, 0 - вниз
    bool cube_direction_x = 1; // 1 - влево, 0 - вправо
    bool game_over = 0;

    draw_line(window, x, x + 100, window->height - 30, 0xFFFFFFFF);
    draw_line(window, x_2, x_2 + 100, 50, 0xFFFFFFFF);


    while (game_over == 0) {

        if(key_state_matrix[wcode_a]) 
            if (x > 0){
                draw_line(window, x, end_x, window->height - 30, 0xFFAAAAAA);
                x -= 2;
                end_x -= 2;
                draw_line(window, x, end_x, window->height - 30, 0xFFFFFFFF);
            }


        if(key_state_matrix[wcode_d]) 
            if (x < window->width - 100){
                draw_line(window, x, end_x, window->height - 30, 0xFFAAAAAA);
                x += 2;
                end_x += 2;
                draw_line(window, x, end_x, window->height - 30, 0xFFFFFFFF);
            }


        if(key_state_matrix[wcode_j]) 
            if (x_2 > 0){
                draw_line(window, x_2, end_x_2, 50, 0xFFAAAAAA);
                x_2 -= 2;
                end_x_2 -= 2;
                draw_line(window, x_2, end_x_2, 50, 0xFFFFFFFF);
            }

        if(key_state_matrix[wcode_l]) 
            if (x_2 < window->width - 100){
                draw_line(window, x_2, end_x_2, 50, 0xFFAAAAAA);
                x_2 += 2;
                end_x_2 += 2;
                draw_line(window, x_2, end_x_2, 50, 0xFFFFFFFF);
            }

        draw_cube(window, cube_x, cube_x+cube_size, cube_y, cube_y+cube_size, 0xFFAAAAAA);

        bool hit_top = collision(cube_x, cube_x + cube_size, x_2, end_x_2) && cube_y <= top_y && cube_y + cube_size > top_y;
        bool hit_bottom = collision(cube_x, cube_x + cube_size, x, end_x) && cube_y + cube_size >= bottom_y && cube_y < bottom_y;


        if (cube_direction_y == 1)
        {
            if (hit_top) {
                cube_direction_y = 0;
                cube_y = top_y + cube_size;

                platform_center = (x_2 + end_x_2)/2;
                cube_center = cube_x + cube_size / 2;

                if (cube_center > platform_center)
                    cube_direction_x = 0;
                else
                    cube_direction_x = 1;
            }
            else if (cube_y > 20) {
                cube_y--;
            }
            else {
                game_over = 1;
            }
        } else {
            if (hit_bottom) {
                cube_direction_y = 1;

                cube_y = bottom_y - cube_size * 2;

                platform_center = (end_x + x)/2;
                cube_center = cube_x + cube_size / 2;

                if (cube_center < platform_center)
                    cube_direction_x = 1;
                else
                    cube_direction_x = 0;
            }
            else if (cube_y + cube_size < window->height) {
                cube_y++;
            }
            else {
                game_over = 1;
            }
        }


        if (cube_direction_x == 1) {
            if (cube_x > 0)
                cube_x--;
            else
                cube_direction_x = 0;
        } else {
            if (cube_x + cube_size < window->width)
                cube_x++;
            else
                cube_direction_x = 1;
        }
        draw_cube(window, cube_x, cube_x+cube_size, cube_y, cube_y+cube_size, 0xFFFFFF);

        compositor_present();
        sleep(5);
    }

    clearframe_win(window, 0xFF000000);

    compositor_present();

    printf("GAME OVER");
}

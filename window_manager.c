/*
 * Copyright (c) 2026 ilizavr & yellowhat
 * SPDX-License-Identifier: MIT
 */


// typedef unsigned long uintptr_t;

// by TINERKOTL
#include "lib/zhirtypes.h"
#include "fonts/font8x16.h"
#include "lib/string.h"

#define MAX_WINDOWS 8
#define MAX_WINDOW_WIDTH 800
#define MAX_WINDOW_HEIGHT 600

u32 (*getchar)();
void *(*alloc)(u32 size);
void (*free)(void *ptr);
void (*printf)(char* fmt, ...);
bool *(*get_key_state_matrix)();
s32 (*get_mouse_x)();
s32 (*get_mouse_y)();
s32 (*get_mouse_left)();
s32 (*get_mouse_right)();
void (*register_function)(char *function_name, void* call, char *description);

struct fb_info* (*fbcon_stop)();

struct fb_info* fb;

typedef struct Window Window;

struct Window{
    s32 x,y;

    s32 width, height;
    bool invisible;
    u32 *buffer, buffer_size;
    bool old_keys[256];

    char *name;
};

bool changed = false;
static bool window_full = false;

u8 *second_buffer = 0;
u32 second_buffer_size = 0;
u32 mouse_x = 0, mouse_y = 0;
u32 old_x, old_y, old_width, old_height;

Window windows[MAX_WINDOWS];
Window *z_order[MAX_WINDOWS];
u32 window_count = 0;
Window *active_window = 0;
u32 active_index = 0;

static u32 arrow_cursor[11][6] = {
    {1, 0, 0, 0, 0, 0},
    {1, 1, 0, 0, 0, 0},
    {1, 1, 1, 0, 0, 0},
    {1, 1, 1, 1, 0, 0},
    {1, 1, 1, 1, 1, 0},
    {1, 1, 1, 1, 0, 0},
    {1, 1, 1, 1, 0, 0},
    {1, 1, 1, 1, 0, 0},
    {1, 0, 1, 1, 1, 0},
    {0, 0, 0, 1, 1, 0},
    {0, 0, 0, 0, 1, 1}
};

static u32 arrow_cursor_height[11][6] = {
    {0, 0, 1, 1, 0, 0},
    {0, 1, 1, 1, 1, 0},
    {1, 1, 1, 1, 1, 1},
    {1, 0, 1, 1, 0, 1},
    {0, 0, 1, 1, 0, 0},
    {0, 0, 1, 1, 0, 0},
    {0, 0, 1, 1, 0, 0},
    {1, 0, 1, 1, 0, 1},
    {1, 1, 1, 1, 1, 1},
    {0, 1, 1, 1, 1, 0},
    {0, 0, 1, 1, 0, 0}
};

static u32 arrow_cursor_width[11][6] = {
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 1, 0, 0, 1, 0},
    {1, 1, 0, 0, 1, 1},
    {1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1},
    {1, 1, 0, 0, 1, 1},
    {0, 1, 0, 0, 1, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0}
};

static const u32 window_icon_1[5][5] = {
    {1, 0, 0, 0, 1},
    {0, 1, 0, 1, 0},
    {0, 0, 1, 0, 0},
    {0, 1, 0, 1, 0},
    {1, 0, 0, 0, 1},
};

static const u32 window_icon_2[5][5] = {
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1},
};

static const u32 window_icon_3[10][8] = {
    {0, 0, 1, 1, 1, 1, 1, 1},
    {0, 0, 1, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 0, 0, 1},
    {1, 0, 1, 0, 1, 0, 0, 1},
    {1, 0, 1, 0, 1, 0, 0, 1},
    {1, 0, 1, 0, 1, 0, 0, 1},
    {1, 0, 1, 0, 1, 0, 0, 1},
    {1, 1, 1, 1, 1, 0, 0, 1},
    {0, 0, 1, 0, 0, 0, 0, 1},
    {0, 0, 1, 1, 1, 1, 1, 1},
};

static Window *free_window_slot() {
    for (u32 i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].buffer == 0)
            return &windows[i];
    }

    return 0;
}

static Window *window_create(char *name) {
    if (window_count >= MAX_WINDOWS)
        return 0;

    Window *window = free_window_slot();

    if (!window)
        return 0;

    memset(window, 0, sizeof(*window));

    window->x = 150;
    window->y = 200;
    window->width = 400;
    window->height = 300;
    window->invisible = 0;
    window->name = name;

    window->buffer_size = window->width*window->height;

    window->buffer = alloc(window->buffer_size * sizeof(u32));
    if(!window->buffer)
        return 0;

    memset(window->buffer, 0, window->buffer_size*sizeof(u32));

    z_order[window_count] = window;

    window_count++;

    return window;
}

static void window_destroy(Window *window) {
    if (!window)
        return;
    
    u32 index = 0;

    while (index < window_count && z_order[index] != window)
        index++;
    
    if (index >= window_count)
        return;

    for (u32 i = index; i + 1 < window_count; i++)
        z_order[i] = z_order[i+1];

    window_count--;

    if(active_window == window) {
        if (window_count > 0)
            active_window = z_order[window_count - 1];
        else
            active_window = 0;
    }

    free(window->buffer);
    window->buffer = 0;
    window->buffer_size = 0;

    changed = true;
}

static void present_framebuffer() {
    u8 *src = second_buffer;
    u8 *dst = (u8*)fb->fb_addr;

    for (u32 i = 0; i < fb->screen_height; i++){
        memcpy(dst + i*fb->screen_pitch,src + i*fb->screen_pitch,fb->screen_pitch);
    }
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

static void clearframe() {
    memset(second_buffer, 0, fb->screen_height*fb->screen_pitch);
}

static void put_pixel_window(Window *window, u32 x, u32 y, u32 color)
{
    if (x >= window->width || y >= window->height)
        return;

    window->buffer[y * window->width + x] = color;
}

static void draw_line_hor_window(Window *window, u32 startx, u32 endx, u32 y, u32 color) {
    for (u32 x = startx; x < endx; x++)
        put_pixel_window(window, x, y, color);
}

static void put_pixel(u32 x, u32 y, u32 color) {
    volatile u32 *pixel = (volatile u32*)(second_buffer + y * fb->screen_pitch + x * fb->bpp);
    *pixel &= 0xFF000000;
    if(fb->bpp == 4) *pixel |= 0xFF000000;
    *pixel |= color;
}

static void put_pixel_cursor(u32 x, u32 y, u32 color) {
    if (x < 0 || y < 0 ||
        x >= (s32)fb->screen_width ||
        y >= (s32)fb->screen_height)
        return;
    volatile u32 *pixel = (volatile u32*)(second_buffer + y * fb->screen_pitch + x * fb->bpp);
    *pixel &= 0xFF000000;
    if(fb->bpp == 4) *pixel |= 0xFF000000;
    *pixel |= color;
}

static void draw_line_hor(u32 startx, u32 endx, u32 y, u32 color) {
    for (u32 x = startx; x < endx; x++)
        put_pixel(x, y, color);
}

static void draw_line_ver(u32 starty, u32 endy, u32 x, u32 color) {
    for (u32 y = starty; y < endy; y++)
        put_pixel(x, y, color);
}

static void draw_char(Window *window, u8 code, u32 x, u32 y, u32 color)
{
    for (u32 row = 0; row < 16; row++) {
        for (u32 col = 0; col < 8; col++) {
            if (font8x16[16*code + row] & (0x80 >> col))
                put_pixel_window(window, x + col, y + row, color);
        }
    }
}

static void draw_string(Window *window, const char *str, u32 x, u32 y, u32 color)
{
    while (*str) {
        draw_char(window, (u8)*str, x, y, color);
        x += 8;
        str++;
    }
}

static void draw_border_window(Window *window, u32 color) {
    u32 *top = &window->buffer[0];
    u32 *bottom = &window->buffer[(window->height - 1) * window->width];

    for (u32 x = 0; x < window->width; x++) {
        top[x] = color;
        bottom[x] = color;
    }

    for (u32 y = 1; y + 1 < window->height; y++) {
        window->buffer[y * window->width] = color;
        window->buffer[y * window->width + window->width - 1] = color;
    }
}

static void window_fill_rect(Window *window, u32 x, u32 y, u32 width, u32 height, u32 color)
{
    u32 *first_row = &window->buffer[y * window->width + x];

    for (u32 i = 0; i < width; i++) 
        first_row[i] = color;


    for (u32 row = 1; row < height; row++) {
        memcpy(&window->buffer[(y + row) * window->width + x], first_row, width * sizeof(u32));
    }
}

static void present_window(Window *window) {
    s32 src_x = 0;
    s32 src_y = 0;
    s32 dst_x = window->x;
    s32 dst_y = window->y;

    s32 copy_width = window->width;
    s32 copy_height = window->height;

    if (dst_x < 0) {
        src_x = -dst_x;
        copy_width -= src_x;
        dst_x = 0;
    }

    if (dst_y < 0) {
        src_y = -dst_y;
        copy_height -= src_y;
        dst_y = 0;
    }

    if (dst_x + copy_width > (s32)fb->screen_width)
        copy_width = fb->screen_width - dst_x;

    if (dst_y + copy_height > (s32)fb->screen_height)
        copy_height = fb->screen_height - dst_y;

    if (copy_width <= 0 || copy_height <= 0)
        return;

    for (s32 y = 0; y < copy_height; y++) {
        u8 *dst = second_buffer + (dst_y + y) * fb->screen_pitch + dst_x * fb->bpp;

        u8 *src =(u8*)&window->buffer[(src_y + y) * window->width + src_x];

        memcpy(dst, src, copy_width * sizeof(u32));
    }
}

static void draw_mouse(u32 (*mouse_now)[6]) {
    for (u32 dy = 0; dy < 11; dy++) {
        for (u32 dx = 0; dx < 6; dx++) {
            if (!mouse_now[dy][dx])
                continue;

            s32 px = (s32)mouse_x + (s32)dx;
            s32 py = (s32)mouse_y + (s32)dy;

            if (px < 0 || py < 0 ||
                px >= (s32)fb->screen_width ||
                py >= (s32)fb->screen_height)
                continue;

            put_pixel_cursor((u32)px, (u32)py, 0x00FF0000);
        }
    }
}

static void compositor_present(u32 (*mouse_now)[6]) {
    clearframe();

    for (u32 i = 0; i < window_count; i++){
        present_window(z_order[i]);
    }

    draw_mouse(mouse_now);
    present_framebuffer();
}

static void filled_circle(Window *window, int cx, int cy, int radius, int color) {
    int radius_squared = radius * radius;

    for (int x = -radius; x <= radius; x++) {
        for (int y = -radius; y <= radius; y++) {
            if (x * x + y * y <= radius_squared) {
                int px = cx + x;
                int py = cy + y;

                put_pixel_window(window, px, py, color);
            }
        }
    }
}

static void window_raise(Window *window) {
    if(window == 0)
        return;

    u32 index = 0;

    while (index < window_count) {
        if (z_order[index] == window)
            break;
        index++;
    }

    if (index >= window_count)
        return;

    for (u32 i = index; i + 1 < window_count; i++){
        z_order[i] = z_order[i+1];
    }

    z_order[window_count-1] = window;
}

static void window_resize(Window *window, s32 new_width, s32 new_height) {
    if (!window)
        return;

    u32 nw = (u32)new_width;
    u32 nh = (u32)new_height;

    if (nw == 0 || nh == 0)
        return;

    if (nw > MAX_WINDOW_WIDTH)
        nw = MAX_WINDOW_WIDTH;
    if (nh > MAX_WINDOW_HEIGHT)
        nh = MAX_WINDOW_HEIGHT;

    if (nw == (u32)window->width && nh == (u32)window->height)
        return;

    u32 *new_buffer = alloc(nw * nh * sizeof(u32));
    if (!new_buffer)
        return;

    memset(new_buffer, 0, nw * nh * sizeof(u32));

    u32 copy_w = nw < (u32)window->width ? nw : (u32)window->width;
    u32 copy_h = nh < (u32)window->height ? nh : (u32)window->height;

    for (u32 y = 0; y < copy_h; y++) {
        memcpy(&new_buffer[y * nw], &window->buffer[y * window->width], copy_w * sizeof(u32));
    }

    free(window->buffer);

    window->buffer = new_buffer;
    window->buffer_size = nw * nh;
    window->width = (s32)nw;
    window->height = (s32)nh;
}

static void window_switch_next()
{
    if (window_count == 0)
        return;

    if (active_window == 0) {
        active_window = z_order[window_count - 1];
        active_index = window_count - 1;
        return;
    }

    u32 index = 0;

    while (index < window_count && z_order[index] != active_window)
        index++;

    if (index >= window_count)
        index = 0;
    else
        index = (index + 1) % window_count;

    active_window = z_order[index];

    window_raise(active_window);

    changed = true;
}

static void resize_active_window(s32 dw, s32 dh)
{
    if (active_window == 0)
        return;

    s32 width = active_window->width + dw;
    s32 height = active_window->height + dh;

    if (width < 20)
        width = 20;

    if (height < 20)
        height = 20;

    if (width > MAX_WINDOW_WIDTH)
        width = MAX_WINDOW_WIDTH;

    if (height > MAX_WINDOW_HEIGHT)
        height = MAX_WINDOW_HEIGHT;

    window_resize(active_window, width, height);
    changed = true;
}

static void draw_icons_window(Window *window) {
    s32 icon_pos = window->width - 70;

    for (u32 dy = 0; dy < 5; dy++) {
        for (u32 dx = 0; dx < 5; dx++) {
            if (!window_icon_1[dy][dx])
                continue;

            u32 px = icon_pos + 45 + dx;
            u32 py = 8 + dy;

            put_pixel_window(window, px, py, 0x00000000);
        }
    }

    for (u32 dy = 0; dy < 5; dy++) {
        for (u32 dx = 0; dx < 5; dx++) {
            if (!window_icon_2[dy][dx])
                continue;

            u32 px = icon_pos + dx;
            u32 py = 8 + dy;

            put_pixel_window(window, px, py, 0x00000000);
        }
    }

    for (u32 dy = 0; dy < 10; dy++) {
        for (u32 dx = 0; dx < 8; dx++) {
            if (!window_icon_3[dy][dx])
                continue;

            u32 px = icon_pos + 20 + dx;
            u32 py = 5 + dy;
                
            put_pixel_window(window, px, py, 0x00000000);
        }
    }
}

static void window_render(Window *window) {
    clearframe_win(window, 0xFFAAAAAA);
    
    window_fill_rect(window, 0, 0, window->width, 20, 0xFFFFFFFF);

    draw_string(window, window->name, 5, 2, 0x000000);

    draw_icons_window(window);

    draw_border_window(window, 0xFFFFFFFF);
}

u32 clamp(s32 value, s32 min, s32 max)
{
    if (value < min)
        return min;

    if (value > max)
        return max;

    return value;
}

static void window_full_screen(Window *window, bool full_screen) {

    if (full_screen){
        old_x = active_window->x;
        old_y = active_window->y;
        old_width = active_window->width;
        old_height = active_window->height;

        window->x = 1;
        window->y = 1;

        window_resize(window, 798, 598); 
    } else {
        window->x = old_x;
        window->y = old_y;

        window_resize(window, old_width, old_height);
    }
    changed = true;
}

INIT void init(void* _resolve_function(char* name))
{
    printf = _resolve_function("_printf");
    getchar = _resolve_function("_getchar");
    fbcon_stop = _resolve_function("_fbcon_stop");
    alloc = _resolve_function("_alloc");
    free = _resolve_function("_free");
    get_mouse_x = _resolve_function("_get_mouse_x");
    get_mouse_y = _resolve_function("_get_mouse_y");
    get_mouse_left = _resolve_function("_get_mouse_left");
    get_mouse_right = _resolve_function("_get_mouse_right");
    register_function = _resolve_function("_register_function");

    register_function("_window_create", window_create, "Create window");
    register_function("_window_render", window_render, "Render window");
    register_function("_clearframe_win", clearframe_win, "Clearframe windows");
    register_function("_window_fill_rect", window_fill_rect, "Draw window fill rect");
    register_function("_draw_string", draw_string, "Draw string");
    register_function("_draw_border_window", draw_border_window, "Draw border for window");
    register_function("_compositor_present", compositor_present, "Draw in main framebuffer");

    get_key_state_matrix = _resolve_function("_get_key_state_matrix");
    bool *key_state_matrix = get_key_state_matrix();

    u8 wcode = ascii_to_keycode['w'];
    u8 dcode = ascii_to_keycode['d'];
    u8 scode = ascii_to_keycode['s'];
    u8 acode = ascii_to_keycode['a'];
    u8 pluscode = ascii_to_keycode['+'];
    u8 mincode = ascii_to_keycode['-'];
    u8 onecode = ascii_to_keycode['1'];

    bool old_window;

    fb = fbcon_stop();

    second_buffer_size = fb->screen_height * fb->screen_pitch;
    second_buffer = alloc(second_buffer_size);

    if (!second_buffer)
        return;

    memset(second_buffer, 0, second_buffer_size);

    clearframe();

    Window *window1 = window_create("window1");
    Window *window2 = window_create("window2");
    // Window *window3 = window_create("window3");

    window_render(window1);
    window_render(window2);
    // window_render(window3);

    active_window = window2;

    u32 (*mouse_now)[6] = arrow_cursor;

    compositor_present(mouse_now);

    s32 old_mouse_x = get_mouse_x();
    s32 old_mouse_y = get_mouse_y();

    s32 old_left = 0;
    s32 old_right = 0;

    u32 drag_offsets_x;
    u32 drag_offsets_y;
    s32 old_width, old_height;
    s32 resize_start_x = 0, resize_start_y = 0;
    s32 old_x, old_y = 0;

    bool dragging = false;
    bool size_changed = false;
    bool on_left, on_right, on_top, on_down;

    while(1){
        mouse_now = arrow_cursor;

        mouse_x = get_mouse_x();
        mouse_y = get_mouse_y();

        s32 left = get_mouse_left();
        s32 right = get_mouse_right();

        bool left_pressed = left && !old_left;
        bool left_hold = left != 0;
        bool left_released = !left && old_left;

        bool right_pressed = right && !old_right;
        bool right_released = !right && old_right;

        if (left_pressed) {
            Window *hit = 0;
            for (u32 i = window_count; i > 0; i--){
                Window *window = z_order[i - 1];

                if (mouse_x >= window->x &&
                    mouse_x < window->x + window->width &&
                    mouse_y >= window->y &&
                    mouse_y < window->y + window->height) {
                    hit = window;
                    break;
                }
            }

            dragging = false;
            size_changed = false;

            if (hit) {
                active_window = hit;
                window_raise(hit);

                drag_offsets_x = mouse_x - hit->x;
                drag_offsets_y = mouse_y - hit->y;

                bool in_tittle = drag_offsets_y < 20;
                bool tittle_button_clicked = false;

                if (in_tittle) {
                    s32 Collapse = hit->width - 70;
                    s32 full_screen = hit->width - 50;
                    s32 close  = hit->width - 25;

                    if ((drag_offsets_x - Collapse) * (drag_offsets_x - Collapse) + (drag_offsets_y - 10) * (drag_offsets_y - 10) <= 49) {
                        hit->invisible = 1;
                        changed = true;
                        tittle_button_clicked = true;
                    }
                    if ((drag_offsets_x - full_screen) * (drag_offsets_x - full_screen) + (drag_offsets_y - 10) * (drag_offsets_y - 10) <= 49) {
                        window_full = !window_full;
                        window_full_screen(hit, window_full);
                        tittle_button_clicked = true;
                    }

                    if ((drag_offsets_x - close) * (drag_offsets_x - close) + (drag_offsets_y - 10) * (drag_offsets_y - 10) <= 49) {
                        window_destroy(hit);
                        tittle_button_clicked = true;
                    }

                    if (hit && !tittle_button_clicked) {
                        dragging = true;
                    }
                } else{
                    on_left = drag_offsets_x < 10;
                    on_right = drag_offsets_x >= hit->width - 10;
                    on_top = drag_offsets_y < 3;
                    on_down = drag_offsets_y >= hit->height - 10;

                    if (on_left || on_down || on_right || on_top) {
                        old_width = hit->width;
                        old_height = hit->height;
                        old_x = hit->x;
                        old_y = hit->y;
                        resize_start_x = mouse_x;
                        resize_start_y = mouse_y;
                        size_changed = true;
                    }
                }
            }
        }

        if (left_hold && active_window) {
            s32 new_x = mouse_x - drag_offsets_x;
            s32 new_y = mouse_y - drag_offsets_y;

            if (dragging) {
                new_x = clamp(new_x, 0, (s32)fb->screen_width - active_window->width);

                new_y = clamp(new_y, 0, (s32)fb->screen_height - active_window->height);

                if (active_window->x != new_x || active_window->y != new_y) {
                    active_window->x = new_x;
                    active_window->y = new_y;
                    changed = true;
                }
            }

            if (size_changed) {
                s32 dx = mouse_x - resize_start_x;
                s32 dy = mouse_y - resize_start_y;
                s32 x = resize_start_x;
                s32 y = resize_start_y;
                s32 width = old_width;
                s32 height = old_height;

                if (on_left) {
                    width = old_width - dx;
                    x = resize_start_x + old_width - width;

                    active_window->x = new_x;
                    mouse_now = arrow_cursor_width;
                }

                if (on_right) {
                    width = old_width + dx;
                    mouse_now = arrow_cursor_width;
                }

                if(on_down) {
                    height = old_height + dy;
                    mouse_now = arrow_cursor_height;
                }

                if (on_top) {
                    height = old_height - dy;
                    y = resize_start_y + old_height - height;
                    mouse_now = arrow_cursor_height;
                }

                if (on_down && on_left) {
                    width = old_width - dx;
                    x = resize_start_x + old_width - width;
                    height = old_height + dy;

                    active_window->x = new_x;
                }

                if (on_down && on_right) {
                    height = old_height + dy;
                    mouse_now = arrow_cursor_height;

                    width = old_width + dx;
                }

                window_resize(active_window, width, height);
                changed = true;
            }
        }

        if (left_released) {
            size_changed = false;
            dragging = false;
        }
        
        if (key_state_matrix[wcode] && active_window->y > 20){
            active_window->y -= 25;
            changed = true;
        }
        
        if (key_state_matrix[scode] && active_window->y+active_window->height < (s32)fb->screen_height){
            active_window->y += 25;
            changed = true;
        }

        if (key_state_matrix[acode] && active_window->x > 0){
            active_window->x -= 25;
            changed = true;
        }

        if (key_state_matrix[dcode] && active_window->x+active_window->width < (s32)fb->screen_width){
            active_window->x += 25;
            changed = true;
        }

        if (key_state_matrix[pluscode]){
            resize_active_window(50,50);
        }

        if (key_state_matrix[mincode]){
            resize_active_window(-50,-50);
        }
        
        if (key_state_matrix[onecode] && !old_window){
            window_switch_next();
        }   

        bool mouse_changed = mouse_x != old_mouse_x || mouse_y != old_mouse_y;

        if (changed || mouse_changed) {
            if (changed)
                window_render(active_window);

            compositor_present(mouse_now);

            old_mouse_x = mouse_x;
            old_mouse_y = mouse_y;
            changed = false;
        }

        old_window = key_state_matrix[onecode];

        old_left = left;
        old_right = right;
    }
}
/*
 * Copyright (c) 2026 ilizavr & yellowhat
 * SPDX-License-Identifier: MIT
 */

// by TINERKOTL (NEUROSLOP)
#include "lib/zhirtypes.h"
#include "lib/string.h"

#define CONSOLE_LINES 20
#define CONSOLE_COLS  40

#define TERM_LINE_HEIGHT 16
#define TERM_LOG_TOP     24

typedef struct Window{
    s32 x,y;

    s32 width, height;
    bool invisible;
    u32 *buffer, buffer_size;
    bool old_keys[256];

    char *name;

    s32 prev_x, prev_y, prev_width, prev_height;
    bool full_screen;
} Window;

Window* (*window_create)(const char *name);
void (*window_render)(Window *window);
bool *(*get_key_state_matrix)();
void (*clearframe_win)(Window *window, u32 color);
void (*window_fill_rect)(Window *window, u32 x, u32 y, u32 width, u32 height, u32 color);
void (*draw_string)(Window *window, const char *str, u32 x, u32 heght, u32 color);
void (*draw_border_window)(Window *window, u32 color);
void (*compositor_present)();

char history[];

typedef struct Console {
    char output[CONSOLE_LINES][CONSOLE_COLS + 1];
    u32 output_line;
    u32 output_col;

    char input[CONSOLE_COLS + 1];
    u32 input_length;

    bool old_keys[256];
} Console;

static void console_scroll(Console *console)
{
    for (u32 y = 1; y < CONSOLE_LINES; y++) {
        for (u32 x = 0; x <= CONSOLE_COLS; x++)
            console->output[y - 1][x] = console->output[y][x];
    }

    memset(console->output[CONSOLE_LINES - 1], 0, CONSOLE_COLS + 1);
    console->output_line = CONSOLE_LINES - 1;
    console->output_col = 0;
}

static void console_newline(Console *console)
{
    console->output_col = 0;
    console->output_line++;

    if (console->output_line >= CONSOLE_LINES)
        console_scroll(console);
}

static void console_putc(Console *console, char c)
{
    if (c == '\n') {
        console_newline(console);
        return;
    }

    if (c == '\r') {
        console->output_col = 0;
        return;
    }

    if ((unsigned char)c < 32)
        return;

    if (console->output_col >= CONSOLE_COLS)
        console_newline(console);

    console->output[console->output_line][console->output_col++] = c;
    console->output[console->output_line][console->output_col] = '\0';

    if (console->output_col >= CONSOLE_COLS)
        console_newline(console);
}

void console_write(Console *console, const char *text)
{
    while (*text)
        console_putc(console, *text++);
}

/* Позже здесь можно разобрать и выполнить введённую команду. */
static void console_execute(Console *console, const char *command)
{
    if (command == "\0")
        return;

}

void console_draw(Console *console, Window *window)
{
    clearframe_win(window, 0x101010);
    window_fill_rect(window, 0, 0, window->width, 20, 0x303030);

    /*
     * Оставляем строку под ввод внизу окна, а историю вывода
     * показываем над ней. Высота окна определяет число видимых строк.
     */
    u32 prompt_y = window->height > 18 ? (u32)window->height - 18 : 0;
    u32 visible_lines = prompt_y > 22 ? (prompt_y - 22) / 16 : 0;

    if (visible_lines > CONSOLE_LINES)
        visible_lines = CONSOLE_LINES;

    u32 first_line = CONSOLE_LINES - visible_lines;

    for (u32 i = 0; i < visible_lines; i++) {
        u32 y = prompt_y - (visible_lines - i) * 16;
        draw_string(window, console->output[first_line + i],
                    8, y, 0x00FF00);
    }

    /* Склеиваем приглашение и ввод в одну строку. */
    char prompt[CONSOLE_COLS + 1];
    prompt[0] = '>';
    prompt[1] = ' ';

    for (u32 i = 0; i < console->input_length; i++)
        prompt[i + 2] = console->input[i];

    prompt[console->input_length + 2] = '\0';

    draw_string(window, prompt, 8, prompt_y, 0xFFFFFF);
    draw_border_window(window, 0x00AA00);
}

void console_update(Console *console, Window *window, bool *keys)
{
    for (u32 code = 0; code < 128; code++) {
        bool pressed = keys[code];
        char c = keyboard_map[code];

        /* Обрабатываем только новое нажатие, не удержание клавиши. */
        if (pressed && !console->old_keys[code]) {
            if (c == '\n' || c == '\r') {
                /* Сохраняем введённую строку в истории вывода. */
                console_write(console, "> ");
                console_write(console, console->input);
                console_putc(console, '\n');

                console_execute(console, console->input);

                console->input_length = 0;
                console->input[0] = '\0';
            } else if (c == '\b' || c == 8) {
                if (console->input_length > 0) {
                    console->input_length--;
                    console->input[console->input_length] = '\0';
                }
            } else if ((unsigned char)c >= 32 &&
                       (unsigned char)c <= 126 &&
                       console->input_length < CONSOLE_COLS - 2) {
                console->input[console->input_length++] = c;
                console->input[console->input_length] = '\0';
            }
        }

        console->old_keys[code] = pressed;
    }

    console_draw(console, window);
}

INIT void init(void* _resolve_function(char* name))
{
    window_create = _resolve_function("_window_create");
    window_render = _resolve_function("_window_render");
    clearframe_win = _resolve_function("_clearframe_win");
    window_fill_rect = _resolve_function("_window_fill_rect");
    draw_string = _resolve_function("_draw_string");
    compositor_present = _resolve_function("_compositor_present");
    draw_border_window = _resolve_function("_draw_border_window");
    get_key_state_matrix = _resolve_function("_get_key_state_matrix");

    bool *key_state_matrix = get_key_state_matrix();

    Console console = {0};
    Window *window = window_create("console");

    console_write(&console, "Console started\n");

    while (1) {
        console_update(&console, window, key_state_matrix);
        compositor_present();
    }
}
/*
 * Copyright (c) 2026 ilizavr & yellowhat
 * SPDX-License-Identifier: MIT
 */

// by TINERKOTL (NEUROSLOP)
#include "../WM_types.h"

#define CONSOLE_COLS 40
#define TERM_LINE_HEIGHT 16
#define TERM_LOG_TOP 24
#define INPUT_INITIAL_CAPACITY 32
#define LINE_INITIAL_CAPACITY 32

Window* (*window_create)(char *name);
void (*window_render)(Window *window);
bool *(*get_key_state_matrix)();
void (*clearframe_win)(Window *window, u32 color);
void (*window_fill_rect)(Window *window, u32 x, u32 y, u32 width, u32 height, u32 color);
void (*draw_string)(Window *window, const char *str, u32 x, u32 heght, u32 color);
void (*draw_border_window)(Window *window, u32 color);
void (*compositor_present)();
bool (*window_is_active)(Window *window);
void (*yield)();
void (*replace_function)(char *function_name, void* newfnc);

void *(*alloc_mem)(u32 size);
bool (*free_mem)(void *ptr);
u32 (*getticks)();

struct task* (*create_task)(char *name, void* function);

typedef struct ConsoleLine {
    struct ConsoleLine *next;
    char *text;
    u32 length;
    u32 capacity;
} ConsoleLine;

typedef struct Console {
    char *input;
    u32 input_length;
    u32 input_capacity;
    bool old_keys[128];

    ConsoleLine *first_line;
    ConsoleLine *last_line;
    u32 line_count;
} Console;

static void zero_bytes(void *buffer, u32 size)
{
    volatile u8 *bytes = buffer;
    while (size--)
        *bytes++ = 0;
}

static u32 text_length(const char *text)
{
    u32 length = 0;
    while (text[length])
        length++;
    return length;
}

static bool text_equals(const char *left, const char *right)
{
    while (*left && *right && *left == *right) {
        left++;
        right++;
    }

    return *left == '\0' && *right == '\0';
}

static bool text_starts_with(const char *text, const char *prefix)
{
    while (*prefix) {
        if (*text != *prefix)
            return false;
        text++;
        prefix++;
    }

    return true;
}

static ConsoleLine *console_create_line(void)
{
    ConsoleLine *line = alloc_mem(sizeof(ConsoleLine));
    if (!line)
        return 0;

    zero_bytes(line, sizeof(ConsoleLine));

    line->capacity = LINE_INITIAL_CAPACITY;
    line->text = alloc_mem(line->capacity);
    if (!line->text) {
        free_mem(line);
        return 0;
    }

    line->text[0] = '\0';
    return line;
}

static bool console_add_line(Console *console)
{
    ConsoleLine *line = console_create_line();
    if (!line)
        return false;

    if (console->last_line)
        console->last_line->next = line;
    else
        console->first_line = line;

    console->last_line = line;
    console->line_count++;
    return true;
}

static void console_clear(Console *console)
{
    ConsoleLine *line = console->first_line;

    while (line) {
        ConsoleLine *next = line->next;
        free_mem(line->text);
        free_mem(line);
        line = next;
    }

    console->first_line = 0;
    console->last_line = 0;
    console->line_count = 0;
    console_add_line(console);
}

static bool console_init(Console *console)
{
    zero_bytes(console, sizeof(Console));

    console->input_capacity = INPUT_INITIAL_CAPACITY;
    console->input = alloc_mem(console->input_capacity);
    if (!console->input)
        return false;

    console->input[0] = '\0';

    if (!console_add_line(console)) {
        free_mem(console->input);
        console->input = 0;
        return false;
    }

    return true;
}

static bool console_append_input(Console *console, char character)
{
    if (console->input_length + 1 >= console->input_capacity) {
        u32 new_capacity = console->input_capacity * 2;
        if (new_capacity <= console->input_capacity)
            return false;

        char *new_input = alloc_mem(new_capacity);
        if (!new_input)
            return false;

        for (u32 i = 0; i <= console->input_length; i++)
            new_input[i] = console->input[i];

        free_mem(console->input);
        console->input = new_input;
        console->input_capacity = new_capacity;
    }

    console->input[console->input_length++] = character;
    console->input[console->input_length] = '\0';
    return true;
}

static bool console_append_output_char(Console *console, char character)
{
    if (character == '\r')
        return true;

    if (character == '\n') {
        return console_add_line(console);
    }

    if (!console->last_line && !console_add_line(console))
        return false;

    if (console->last_line->length >= CONSOLE_COLS &&
        !console_add_line(console))
        return false;

    ConsoleLine *line = console->last_line;

    if (line->length + 1 >= line->capacity) {
        u32 new_capacity = line->capacity * 2;
        if (new_capacity <= line->capacity)
            return false;

        char *new_text = alloc_mem(new_capacity);
        if (!new_text)
            return false;

        for (u32 i = 0; i <= line->length; i++)
            new_text[i] = line->text[i];

        free_mem(line->text);
        line->text = new_text;
        line->capacity = new_capacity;
    }

    line->text[line->length++] = character;
    line->text[line->length] = '\0';
    return true;
}

static void console_write(Console *console, const char *text)
{
    while (*text)
        console_append_output_char(console, *text++);
}

static void console_write_u32(Console *console, u32 value)
{
    char digits[11];
    u32 count = 0;

    do {
        digits[count++] = '0' + value % 10;
        value /= 10;
    } while (value && count < sizeof(digits));

    while (count)
        console_append_output_char(console, digits[--count]);
}

static void console_execute(Console *console, const char *command)
{
    if (!command[0])
        return;

    if (text_equals(command, "help")) {
        console_write(console, "Commands:");
        console_append_output_char(console, '\n');
        console_write(console, "clear - clear terminal");
        console_append_output_char(console, '\n');
        console_write(console, "echo TEXT - print text");
        console_append_output_char(console, '\n');
        console_write(console, "uptime - show system ticks");
        console_append_output_char(console, '\n');
        console_write(console, "printf - print text");
        console_append_output_char(console, '\n');
        console_write(console, "getchar - print pressed key on keyboard");
        console_append_output_char(console, '\n');
        return;
    }

    if (text_equals(command, "clear")) {
        console_clear(console);
        return;
    }

    if (text_starts_with(command, "echo ")) {
        console_write(console, command + 5);
        console_append_output_char(console, '\n');
        return;
    }

    if (text_equals(command, "uptime")) {
        console_write(console, "Second: ");
        console_write_u32(console, (getticks ? getticks() : 0) / 1000);
        console_append_output_char(console, '\n');
        return;
    }

    if (text_equals(command, "printf")) {
        return;
    }

    if (text_equals(command, "getchar")) {
        return;
    }

    console_write(console, "Command not found: ");
    console_write(console, command);
    console_append_output_char(console, '\n');
}

static void console_submit(Console *console)
{
    console_write(console, "> ");
    console_write(console, console->input);
    console_append_output_char(console, '\n');

    console_execute(console, console->input);

    console->input_length = 0;
    console->input[0] = '\0';
}

static void console_draw(Console *console, Window *window)
{
    if (window->width <= 2 || window->height <= 21)
        return;

    window_fill_rect(window, 1, 20, window->width - 2, window->height - 21, 0xFF101010);

    u32 prompt_y = (u32)window->height - 18;
    u32 visible_lines = 0;
    u32 skip = 0;

    if (prompt_y > TERM_LOG_TOP)
        visible_lines = (prompt_y - TERM_LOG_TOP) / TERM_LINE_HEIGHT;

    if (console->line_count > visible_lines)
        skip = console->line_count - visible_lines;

    ConsoleLine *line = console->first_line;
    while (line && skip > 0) {
        line = line->next;
        skip--;
    }

    for (u32 row = 0; line && row < visible_lines; row++, line = line->next)
        draw_string(window, line->text, 8, TERM_LOG_TOP + row * TERM_LINE_HEIGHT, 0x00FF00);

    char prompt[CONSOLE_COLS + 1];
    u32 max_input = CONSOLE_COLS - 2;
    u32 start = console->input_length > max_input ? console->input_length - max_input : 0;
    u32 length = console->input_length - start;

    prompt[0] = '>';
    prompt[1] = ' ';
    for (u32 i = 0; i < length; i++)
        prompt[i + 2] = console->input[start + i];

    prompt[length + 2] = '\0';
    draw_string(window, prompt, 8, prompt_y, 0xFFFFFF);
}

static void console_update(Console *console, Window *window, bool *keys)
{
    bool focused = window_is_active(window);

    for (u32 code = 0; code < 128; code++) {
        bool pressed = keys[code];
        bool just_pressed = pressed && !console->old_keys[code];
        char character = keyboard_map[code];

        if (focused && just_pressed) {
            if (character == '\n' || character == '\r') {
                console_submit(console);
            } else if (character == '\b') {
                if (console->input_length > 0) {
                    console->input_length--;
                    console->input[console->input_length] = '\0';
                }
            } else if (character >= 32 && character <= 126) {
                if (keys[42] || keys[54])
                    character = keyboard_map_shift[code];

                if (character >= 32 && character <= 126)
                    console_append_input(console, character);
            }
        }

        console->old_keys[code] = pressed;
    }

    console_draw(console, window);
}

static void init_terminal()
{
    bool *key_state_matrix = get_key_state_matrix();

    Console console;
    if (!console_init(&console))
        return;

    Window *window = window_create("Console");

    window_render(window);
    console_write(&console, "ZhirOS Terminal");
    console_append_output_char(&console, '\n');
    console_write(&console, "Type help for available commands.");
    console_append_output_char(&console, '\n');

    while (1) {
        console_update(&console, window, key_state_matrix);
        compositor_present();
        yield();
    }
}

INIT void init(void* _resolve_function(char* name))
{
    window_create = _resolve_function("_window_create");
    window_render = _resolve_function("_window_render");
    window_fill_rect = _resolve_function("_window_fill_rect");
    draw_string = _resolve_function("_draw_string");
    compositor_present = _resolve_function("_compositor_present");
    get_key_state_matrix = _resolve_function("_get_key_state_matrix");
    window_is_active = _resolve_function("_window_is_active");
    create_task = _resolve_function("_create_task");
    alloc_mem = _resolve_function("_alloc");
    free_mem = _resolve_function("_free");
    getticks = _resolve_function("_getticks");
    yield = _resolve_function("_yield");
    replace_function = _resolve_function("_replace_function");


    create_task("terminal", init_terminal);
}
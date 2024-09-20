#ifndef __TERMINAL_CONTROL_H__
#define __TERMINAL_CONTROL_H__

#include <stdint.h>
#include <stdlib.h>

#define WINDOW_MIN_WIDTH 30
#define TERMINAL_MIN_SIZE 10

#define CLEAR_SCREEN "\x1b[H\x1b[J"
#define MOVE_CURSOR "\x1b[%zu;%zuH"
#define DISABLE_CURSOR "\x1b[?25l"
#define ENABLE_CURSOR "\x1b[?25h"
#ifdef USE_UNICODE
    #define PIPE "║"
    #define PIPE2 "│"
    #define HIGHLIGHT_START "\x1b[48;5;46m\x1b[30m"
    #define HIGHLIGHT_RED "\x1b[41m\x1b[37m"
#else
    #define PIPE "|"
    #define PIPE2 "|"
    #define HIGHLIGHT_START "\x1b[7m"
    #define HIGHLIGHT_RED ""
#endif
#define HIGHLIGHT_END "\x1b[0m"

void init_terminal();
void reset_terminal();
int32_t select_option(char *headers[], size_t header_size, char *options[], size_t option_size);
char *get_raw_str(char *prompt, size_t max_len);
char *get_str(char *header, size_t max_len, int32_t (*filter)(int), char *placeholder);
uint32_t get_int(char *prompt, size_t max_length, char *placeholder);
void popup(char *h1, char *h2, char *h3);
void press_any_key();
void print_centered(char *before, char *str, size_t max_len, char *after);
int32_t rollover(int32_t current_val, int32_t max_val, int32_t increment);
void cleanup_and_exit();

#endif /* __TERMINAL_CONTROL_H__ */
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#include "terminal-control.h"

#define IOCTL_ERROR -1
#define TERM_DECOR_SPACE 4
#define MENU_END_SPACE 3
#define BORDER_STR_LIMIT 10

#define ESC_SEQ_START 0x1b
#define ESC_SEQ_BRACKET 0x5B
#define ESC_SEQ_MIN_FINAL 0x40
#define ESC_SEQ_MAX_FINAL 0x7E
#define ESC_SEQ_MAX_SIZE 4

#define INVALID_OPTION -1
#define KEYPRESS_TIMEOUT UINT32_MAX

#define VALUE_INCREASE +1
#define VALUE_DECREASE -1
#define VALUE_MAX_GET_STR 2

#define SELECTED_INPUT_FIELD 0
#define SELECTED_OK_BUTTON 1
#define SELECTED_CANCLE_BUTTON 2

#define INPUT_DIALOG_OFFSET_Y 4
#define INPUT_DIALOG_OFFSET_X 3

#define INVALID_INT UINT32_MAX
#define BASE_10 10

typedef enum key_codes
{
    KEY_BACKSPACE_ALT = 0x8,
    KEY_ENTER = 0xA,
    KEY_QUIT_1 = 'Q',
    KEY_QUIT_2 = 'q',
    KEY_BACKSPACE = 0x7F,
    KEY_UP_ARROW = 0x415B1B,
    KEY_DOWN_ARROW = 0x425B1B,
    KEY_RIGHT_ARROW = 0x435B1B,
    KEY_LEFT_ARROW = 0x445B1B,
} key_codes_t;

char terminal_print_buffer[BUFSIZ] = {'\0'};
static struct termios terminal_original_termios;
static bool terminal_termios_initialized = false;
unsigned short int terminal_rows = 0, terminal_cols = 0;

static inline void check_initialization();
static bool terminal_size_changed();
static uint32_t get_keypress();
static void print_menu(char *[], size_t, char *[], size_t, int32_t);
static void print_border(char *, char *, size_t, char *);
static void print_input_dialog(char *, char *, size_t, int32_t, char *);

static inline void check_initialization()
{
    if (terminal_termios_initialized == false)
    {
        perror("termios not initialized");
        cleanup_and_exit();
    }

    return;
}

static bool terminal_size_changed()
{
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == IOCTL_ERROR)
    {
        perror("ioctl error");
        cleanup_and_exit();
    }

    if (terminal_rows != ws.ws_row || terminal_cols != ws.ws_col)
    {
        terminal_rows = ws.ws_row;
        terminal_cols = ws.ws_col;
        return true;
    }

    return false;
}

static void print_border(char *start, char *middle, size_t middle_count, char *end)
{
    size_t pos = 0;
    size_t start_length = 0;
    size_t middle_length = 0;
    size_t end_length = 0;
    size_t total_length = 0;
    char *line = NULL;

    if (start == NULL && middle == NULL && end == NULL)
    {
        printf("\n");
        return;
    }

    start_length = (start != NULL) ? strnlen(start, BORDER_STR_LIMIT) : 0;
    middle_length = (middle != NULL) ? strnlen(middle, BORDER_STR_LIMIT) : 0;
    end_length = (end != NULL) ? strnlen(end, BORDER_STR_LIMIT) : 0;
    total_length = start_length + middle_count * middle_length + end_length;

    line = (char *)malloc(total_length + 1);

    if (line == NULL)
    {
        perror("Failed to allocate memory");
        return;
    }

    memcpy(line + pos, start, start_length);
    pos += start_length;

    for (size_t i = 0; i < middle_count; ++i)
    {
        memcpy(line + pos, middle, middle_length);
        pos += middle_length;
    }

    memcpy(line + pos, end, end_length);
    pos += end_length;
    line[total_length] = '\0';

    puts(line);
    free(line);
    line = NULL;

    return;
}

/****************************************************************************
 * Name: print_menu
 * Input:
 *   char *headers[]       An array of strings representing the header.
 *   size_t header_size    The size of header.
 *   char *options[]       An array of strings representing the menu options.
 *   size_t option_size    The size of options.
 *   int32_t selected      The index of the currently selected menu option.
 * Return:
 *   None
 * Description:
 *   This function displays a menu in the terminal with the given options. It prints
 *   the options in a bordered box, highlighting the currently selected option.
 *   The use of Unicode characters for borders is conditional on the USE_UNICODE macro.
 ****************************************************************************/
static void print_menu(char *headers[], size_t header_size, char *options[], size_t option_size,
                       int32_t selected)
{
    int32_t i = 0;
    size_t max_length = 0;
    size_t str_length = 0;

    int32_t start_index = 0;
    int32_t end_index = 0;

    if (terminal_cols < TERMINAL_MIN_SIZE || terminal_rows < TERMINAL_MIN_SIZE)
    {
        printf(CLEAR_SCREEN "Window Size too small (%dx%d), resize window", terminal_cols,
               terminal_rows);
        fflush(stdout);
        return;
    }

    /* subset of options to display */
    start_index = 0;
    end_index = (option_size < (size_t)terminal_rows - TERM_DECOR_SPACE - header_size)
                    ? (int32_t)option_size
                    : (int32_t)(terminal_rows - TERM_DECOR_SPACE - header_size);

    /* move the subset down if currently selected is not in view */
    while (selected >= end_index - MENU_END_SPACE)
    {
        end_index++;
        start_index++;
    }

    /* move the subset up if end index exceeds menu size */
    while (end_index > (int32_t)option_size)
    {
        end_index--;
        start_index--;
    }

    /* Find the length of maximum length string */
    for (i = 0; i < (int32_t)header_size; i++)
    {
        str_length = strnlen(headers[i], terminal_cols - TERM_DECOR_SPACE);
        max_length = (max_length > str_length) ? max_length : str_length;
    }

    for (i = 0; i < (int32_t)option_size; i++)
    {
        str_length = strnlen(options[i], terminal_cols - TERM_DECOR_SPACE);
        max_length = (max_length > str_length) ? max_length : str_length;
    }

    max_length = (max_length < WINDOW_MIN_WIDTH) ? WINDOW_MIN_WIDTH : max_length;

    printf(CLEAR_SCREEN);
#ifdef USE_UNICODE
    print_border("╔═", "═", max_length, "═╗");
#else
    print_border("+-", "-", max_length, "-+");
#endif

    for (i = 0; (size_t)i < header_size; i++)
    {
        print_centered(PIPE " ", headers[i], max_length, " " PIPE "\n");
    }

#ifdef USE_UNICODE
    print_border("╠═", "═", max_length, "═╣");
#else
    print_border("+-", "-", max_length, "-+");
#endif

    /* show the subset of options */
    for (i = start_index; i < end_index; i++)
    {
        if (i == selected)
        {
            printf(PIPE " " HIGHLIGHT_START "%-*.*s" HIGHLIGHT_END " " PIPE "\n", (int)max_length,
                   (int)max_length, options[i]);
        }
        else
        {
            printf(PIPE " %-*.*s " PIPE "\n", (int)max_length, (int)max_length, options[i]);
        }
    }

#ifdef USE_UNICODE
    print_border("╚═", "═", max_length, "═╝");
#else
    print_border("+-", "-", max_length, "-+");
#endif
    fflush(stdout);

    return;
}

/****************************************************************************
 * Name: get_keypress
 * Input:
 *   None
 * Return:
 *   uint32_t   The value of the key pressed as a 32-bit unsigned integer.
 *              If no key is pressed within the timeout period, returns KEYPRESS_TIMEOUT.
 * Description:
 *   This function waits for a keypress from the standard input (stdin) and
 *   returns the key's value as a 32-bit unsigned integer. If no key is pressed
 *   within a predefined timeout period (10 milliseconds), it returns KEYPRESS_TIMEOUT
 *   to indicate that no input was received.
 ****************************************************************************/
static uint32_t get_keypress()
{
    union {
        unsigned char c[ESC_SEQ_MAX_SIZE];
        uint32_t i;
    } buf = {0};
    ssize_t result = 0;
    size_t bytes_read = 0;
    struct timeval timeout = {0};
    fd_set read_fds;

    timeout.tv_usec = 10000;
    FD_ZERO(&read_fds);
    FD_SET(STDIN_FILENO, &read_fds);

    check_initialization();

    /**
     * Use select() to monitor the file descriptors. It will wait for input on
     * STDIN or until the timeout occurs. FD_ISSET checks if STDIN_FILENO
     * (standard input) is ready for reading.
     */
    if (select(STDIN_FILENO + 1, &read_fds, NULL, NULL, &timeout) > 0 &&
        FD_ISSET(STDIN_FILENO, &read_fds))
    {
        bytes_read = 0;

        while (bytes_read < sizeof(buf.c))
        {
            result = read(STDIN_FILENO, &buf.c[bytes_read], 1);

            if (result < 1)
            {
                return KEYPRESS_TIMEOUT;
            }

            bytes_read++;

            if (buf.c[0] != ESC_SEQ_START || bytes_read >= ESC_SEQ_MAX_SIZE)
            {
                break;
            }

            if (buf.c[1] == ESC_SEQ_BRACKET && buf.c[2] >= ESC_SEQ_MIN_FINAL &&
                buf.c[2] <= ESC_SEQ_MAX_FINAL)
            {
                break;
            }
        }

        return buf.i;
    }

    return KEYPRESS_TIMEOUT;
}

void init_terminal()
{
    struct termios raw = {0};

    if (terminal_termios_initialized == false)
    {
        setvbuf(stdout, terminal_print_buffer, _IOFBF, BUFSIZ);
        tcgetattr(STDIN_FILENO, &terminal_original_termios);
        memcpy(&raw, &terminal_original_termios, sizeof(terminal_original_termios));
        raw.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);
        terminal_termios_initialized = true;

        printf(DISABLE_CURSOR);
        fflush(stdout);
    }

    return;
}

void reset_terminal()
{
    if (terminal_termios_initialized == true)
    {
        tcsetattr(STDIN_FILENO, TCSANOW, &terminal_original_termios);
        setvbuf(stdout, NULL, _IOLBF, 0);
    }

    return;
}

/****************************************************************************
 * Name: select_option
 * Input:
 *   char *headers[]       An array of strings representing the header.
 *   size_t header_size    The size of header.
 *   char *options[]       An array of strings representing the menu options.
 *   size_t option_size    The size of options.
 * Return: On success, returns the index of the selected option.
 *         If the user exits the menu or no valid selection is made, returns -1.
 * Description: Displays a menu to the user based on the provided options and allows
 *              the user to navigate and select an option using keyboard input.
 *              The function handles arrow key inputs for navigation and the Enter
 *              key for selection. If the user presses 'q' or 'Q', the menu exits
 *              without making a selection.
 ****************************************************************************/
int32_t select_option(char *headers[], size_t header_size, char *options[], size_t option_size)
{
    bool running = true;
    bool reprint = true;
    uint32_t keypress = 0;
    int32_t selected = 0;

    check_initialization();

    if (options == NULL || option_size == 0)
    {
        return INVALID_OPTION;
    }

    while (running)
    {
        if (terminal_size_changed() || reprint)
        {
            print_menu(headers, header_size, options, option_size, selected);
        }

        reprint = true;
        keypress = get_keypress();

        switch (keypress)
        {
            case KEYPRESS_TIMEOUT:
                reprint = false;
                break;
            case KEY_UP_ARROW:
            case KEY_LEFT_ARROW:
                selected = rollover(selected, option_size - 1, VALUE_DECREASE);
                break;
            case KEY_DOWN_ARROW:
            case KEY_RIGHT_ARROW:
                selected = rollover(selected, option_size - 1, VALUE_INCREASE);
                break;
            case KEY_ENTER:
                running = false;
                return selected;
                break;
            case KEY_QUIT_1:
            case KEY_QUIT_2:
                running = false;
                break;
            default:
                break;
        }
    }

    return INVALID_OPTION;
}

/****************************************************************************
 * Name: popup
 * Input:
 *   char *h1     Pointer to the first header string to be displayed.
 *   char *h2     Pointer to the second header string to be displayed.
 *   char *option Pointer to the selected option string.
 * Return:
 *   None.
 * Description:
 *   This function displays a popup menu with two header strings and an option
 *   string provided as input parameters and waits for the user to press enter
 *   or 'Q'/'q'.
 ****************************************************************************/
void popup(char *h1, char *h2, char *option)
{
    if (h1 == NULL || h2 == NULL || option == NULL)
    {
        return;
    }

    char *header[2] = {h1, h2};
    char *options[1] = {option};
    select_option(header, sizeof(header) / sizeof(header[0]), options,
                  sizeof(options) / sizeof(options[0]));

    return;
}

char *get_raw_str(char *prompt, size_t max_len)
{
    char *buf = NULL;
    size_t current_pos = 0;
    uint32_t keypress = 0;
    bool running = false;

    if (max_len == 0)
    {
        return NULL;
    }

    printf(CLEAR_SCREEN);

    if (prompt != NULL)
    {
        puts(prompt);
    }

    fflush(stdout);

    buf = (char *)malloc(max_len * sizeof(char));

    if (buf == NULL)
    {
        return NULL;
    }

    running = true;
    buf[0] = '\0';
    printf(">> " ENABLE_CURSOR);
    fflush(stdout);

    while (running)
    {
        keypress = get_keypress();

        if (isprint((char)keypress))
        {
            if (current_pos < max_len - 1)
            {
                buf[current_pos++] = (char)keypress;
                buf[current_pos] = '\0';
                putchar(keypress);
                fflush(stdout);
            }
            continue;
        }

        switch (keypress)
        {
            case KEY_BACKSPACE:
            case KEY_BACKSPACE_ALT:

                if (current_pos > 0)
                {
                    buf[--current_pos] = '\0';
                    printf(CLEAR_SCREEN);

                    if (prompt != NULL)
                    {
                        puts(prompt);
                    }

                    printf(">> %s", buf);
                    fflush(stdout);
                }

                break;
            case KEY_ENTER:
                running = false;
                printf(DISABLE_CURSOR "\n");
                fflush(stdout);
                break;
            default:
                break;
        }
    }

    return buf;
}

/****************************************************************************
 * Name: get_str
 * Input:
 *   char *header            Pointer to a string to be displayed as the prompt/
 *                           header for user input.
 *   size_t max_len          Maximum length of the input string, including the
 *                           null terminator.
 *   int32_t (*filter)(int)  Function pointer to a filter function that determines
 *                           whether a character is valid. This function takes an
 *                           int (character) and returns a non-zero value if the
 *                           character is valid.
 *   char *placeholder       Pointer to a string to be used as initial input or
 *                           placeholder text. This string is pre-filled into the
 *                           input buffer if not NULL.
 * Return:
 *   char *           Pointer to a dynamically allocated string containing the user input.
 *                    Returns NULL if memory allocation fails or if the user exits without
 *                    providing valid input.
 * Description:
 *   This function prompts the user to enter a string, with options for handling
 *   special characters and user input constraints. It manages the input buffer, validates
 *   input characters using the provided filter function, and supports basic text editing
 *   commands such as backspace.
 ****************************************************************************/
char *get_str(char *header, size_t max_len, int32_t (*filter)(int), char *placeholder)
{
    bool reprint = true;
    bool running = true;
    uint32_t keypress = 0;
    int32_t selected = 0;
    size_t current_pos = 0;
    char *input = NULL;
    char *help = NULL;

    check_initialization();

    input = (char *)calloc(max_len, sizeof(char));

    if (input == NULL)
    {
        fprintf(
            stderr,
            "Memory allocation failed\nCould not allocate memory to store user input string.\n");
        press_any_key();

        return NULL;
    }

    if (placeholder != NULL)
    {
        for (size_t i = 0; i < max_len - 1; i++)
        {
            if (filter(placeholder[i]) && placeholder[i] != '\0')
            {
                input[current_pos++] = placeholder[i];
            }
            else
            {
                break;
            }
        }
    }

    printf(ENABLE_CURSOR);

    while (running)
    {
        if (terminal_size_changed() || reprint)
        {
            print_input_dialog(header, input, max_len - 1, selected, help);
        }

        help = NULL;
        reprint = true;
        keypress = get_keypress();

        if (selected == SELECTED_INPUT_FIELD)
        {
            if (filter((char)keypress))
            {
                if (current_pos < max_len - 1)
                {
                    input[current_pos++] = (char)keypress;
                }
                else
                {
                    help = "Input limit reached";
                }
            }
            else
            {
                help = "Enter valid character";
            }
        }

        switch (keypress)
        {
            case KEY_UP_ARROW:
            case KEY_LEFT_ARROW:
                selected = rollover(selected, VALUE_MAX_GET_STR, VALUE_DECREASE);
                help = NULL;
                break;
            case KEY_DOWN_ARROW:
            case KEY_RIGHT_ARROW:
                selected = rollover(selected, VALUE_MAX_GET_STR, VALUE_INCREASE);
                help = NULL;
                break;
            case KEY_ENTER:
                running = false;
                break;
            case KEY_BACKSPACE:
            case KEY_BACKSPACE_ALT:
                help = "Nothing in buffer to remove";

                if (selected == SELECTED_INPUT_FIELD && current_pos > 0)
                {
                    input[--current_pos] = '\0';
                    help = NULL;
                }

                break;
            case KEYPRESS_TIMEOUT:
                reprint = false;
                break;
            default:
                break;
        }
    }

    printf(DISABLE_CURSOR);

    if (selected == SELECTED_INPUT_FIELD || selected == SELECTED_OK_BUTTON)
    {
        return input;
    }

    free(input);
    input = NULL;

    return input;
}

/****************************************************************************
 * Name: print_input_dialog
 * Input:
 *   char *header     Pointer to a string to be displayed as the header or title
 *                    of the dialog box.
 *   char *input      Pointer to a string representing the current user input.
 *   size_t max_len   Maximum length of the input field and dialog box width.
 *   int32_t selected Index indicating the currently selected option
 *                        0: input field,
 *                        1: OK button,
 *                        2: Cancel button.
 *   char *help       Pointer to a string containing help or error messages to be displayed.
 * Return:
 *   None.
 * Description:
 *   This function displays a dialog box in the terminal with a header, an input field,
 *   and two options (OK and Cancel).
 ****************************************************************************/
static void print_input_dialog(char *header, char *input, size_t max_len, int32_t selected,
                               char *help)
{
    size_t terminal_width = 0;
    size_t str_len = 0;

    terminal_width = terminal_cols - TERM_DECOR_SPACE;
    str_len = strnlen(header, (max_len > terminal_width) ? max_len : terminal_width);
    max_len = (max_len > str_len) ? max_len : str_len;
    max_len = (max_len > WINDOW_MIN_WIDTH) ? max_len : WINDOW_MIN_WIDTH;

    printf(CLEAR_SCREEN);
#ifdef USE_UNICODE
    print_border("╔═", "═", max_len, "═╗");
    print_centered("║ ", header, max_len, " ║\n");
    print_border("╠═", "═", max_len, "═╣");
#else
    print_border("+-", "-", max_len, "-+");
    print_centered("| ", header, max_len, " |\n");
    print_border("+-", "-", max_len, "-+");
#endif

    if (selected == SELECTED_INPUT_FIELD)
    {
        printf(PIPE " " HIGHLIGHT_START "%-*.*s" HIGHLIGHT_END " " PIPE "\n", (int)max_len,
               (int)max_len, input);
    }
    else
    {
        printf(PIPE " %-*.*s " PIPE "\n", (int)max_len, (int)max_len, input);
    }

#ifdef USE_UNICODE
    print_border("╚═", "═", max_len, "═╝");
#else
    print_border("+-", "-", max_len, "-+");
#endif

    if (selected == SELECTED_OK_BUTTON)
    {
        print_centered("  " HIGHLIGHT_START, "[  OK  ]", max_len / 2, HIGHLIGHT_END);
    }
    else
    {
        print_centered("  ", "[  OK  ]", max_len / 2, "");
    }

    if (selected == SELECTED_CANCLE_BUTTON)
    {
        print_centered(HIGHLIGHT_START, "[Cancel]", max_len - max_len / 2, HIGHLIGHT_END "  ");
    }
    else
    {
        print_centered("", "[Cancel]", max_len - max_len / 2, "  ");
    }

    if (help != NULL)
    {
        printf("\n\n*%s", help);
    }

    /* Move cursor to appropriate position */
    printf(MOVE_CURSOR, (size_t)INPUT_DIALOG_OFFSET_Y,
           strnlen(input, max_len) + INPUT_DIALOG_OFFSET_X);

    fflush(stdout);

    return;
}

/****************************************************************************
 * Name: get_int
 * Input:
 *   char *prompt       Pointer to a string to be displayed as a prompt to the user.
 *   size_t max_length  Maximum length of the input digit.
 *   char *placeholder  Pointer to a string to be used as placeholder
 *                      in the input field. This may be NULL.
 * Return:
 *   uint32_t           The integer value parsed from the user input, or UINT32_MAX
 *                      if the input is invalid or conversion fails.
 * Description:
 *   This function prompts the user to enter an integer value. It uses the `get_str`
 *   function to obtain a string input from the user, and then attempts to parse it as
 *   an unsigned integer.
 ****************************************************************************/
uint32_t get_int(char *prompt, size_t max_length, char *placeholder)
{
    char *str = NULL;
    char *endptr = NULL;
    unsigned long result = 0;

    result = INVALID_INT;
    str = get_str(prompt, max_length, &isdigit, placeholder);

    if (str != NULL && str[0] != '\0')
    {
        errno = 0;
        result = strtoul(str, &endptr, BASE_10);

        if (errno != 0 || *endptr != '\0' || result > UINT32_MAX)
        {
            result = INVALID_INT;
        }
    }

    free(str);
    str = NULL;

    return (uint32_t)result;
}

void press_any_key()
{
    printf("\nPress any key to continue...\n");
    fflush(stdout);

    while (get_keypress() == KEYPRESS_TIMEOUT)
    {
        usleep(10000);
    }

    return;
}

void print_centered(char *before, char *str, size_t max_len, char *after)
{
    size_t str_len = 0;
    size_t pad = 0;

    str_len = strnlen(str, max_len);
    pad = (max_len - str_len) / 2;
    printf("%s%*.*s%*s%s", before, (int)(pad + str_len), (int)(pad + str_len), str,
           (int)(max_len - pad - str_len), "", after);
    return;
}

int32_t rollover(int32_t current_val, int32_t max_val, int32_t increment)
{
    current_val += increment;

    if (current_val > max_val)
    {
        return 0;
    }
    else if (current_val < 0)
    {
        return max_val;
    }

    return current_val;
}

void cleanup_and_exit()
{
    reset_terminal();
    printf(ENABLE_CURSOR);
    fflush(stdout);
    exit(0);

    return;
}
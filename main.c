#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitmap.h"
#include "terminal-control.h"

#define HEADER_SIZE 1
#define BITMAP_COUNT 5
#define MAX_INPUT_SIZE 1024
#define INITIAL_CAPACITY 100
#define MENU_SIZE 10
#define BETWEEN(val, min, max) ((val) > (min) && (val) < (max))

static char **bitmap_options();
void handle_change_capacity(void);
void handle_add_value(void);
void handle_del_value(void);
void handle_print_bitmap(void);
void handle_invert_bitmap(void);
void handle_or_bitmap(void);
void handle_and_bitmap(void);
void handle_parse_bitmap(void);
void handle_clone_bitmap(void);
void cleanup_bitmaps(void);
void exit_command(int n);
char *get_allocated(const char *format, ...);

typedef struct MenuOption
{
    const char *description;
    void (*action)(void);
} MenuOption_t;

struct bitmap *bitmaps[BITMAP_COUNT] = {NULL};

void exit_command(int n)
{
    uint32_t i = 0;
    char **options = NULL;

    printf(ENABLE_CURSOR);
    fflush(stdout);
    reset_terminal();

    for (i = 0; i < BITMAP_COUNT; i++)
    {
        bitmap_destroy(bitmaps[i]);
        bitmaps[i] = NULL;
    }

    options = bitmap_options();

    for (i = 0; i < BITMAP_COUNT; i++)
    {
        free(options[i]);
        options[i] = NULL;
    }

    if (n == EXIT_SUCCESS || n == SIGINT)
    {
        exit(EXIT_SUCCESS);
    }

    exit(EXIT_FAILURE);

    return;
}

int main(void)
{
    bool running = false;
    int32_t i = 0;
    int32_t choice = 0;
    MenuOption_t *menu[MENU_SIZE] = {NULL};
    const char *menu_headers[1] = {NULL};
    const char *menu_descriptions[MENU_SIZE] = {NULL};

    menu[0] = &(MenuOption_t){"Change capacity", handle_change_capacity};
    menu[1] = &(MenuOption_t){"Add value to a bitmap", handle_add_value};
    menu[2] = &(MenuOption_t){"Delete value from a bitmap", handle_del_value};
    menu[3] = &(MenuOption_t){"Invert a bitmap", handle_invert_bitmap};
    menu[4] = &(MenuOption_t){"OR two bitmaps", handle_or_bitmap};
    menu[5] = &(MenuOption_t){"AND two bitmaps", handle_and_bitmap};
    menu[6] = &(MenuOption_t){"Parse bitmap from string", handle_parse_bitmap};
    menu[7] = &(MenuOption_t){"Print all bitmaps", handle_print_bitmap};
    menu[8] = &(MenuOption_t){"Clone bitmap", handle_clone_bitmap};
    menu[9] = &(MenuOption_t){"Exit", cleanup_bitmaps};

    menu_headers[0] = "Test Bitmap";

    for (i = 0; i < MENU_SIZE; i++)
    {
        menu_descriptions[i] = menu[i]->description;
    }

    signal(SIGINT, &exit_command); /* Catch Ctrl+C */
    init_terminal();

    /*  Initialize the bitmaps */
    for (i = 0; i < BITMAP_COUNT; i++)
    {
        bitmaps[i] = bitmap_create(INITIAL_CAPACITY);

        if (bitmaps[i] == NULL)
        {
            printf("Failed to create bitmap %" PRId32 "\n", i + 1);
            exit_command(EXIT_FAILURE);
        }
    }

    running = true;

    while (running)
    {
        choice = select_option((char **)menu_headers, 1, (char **)menu_descriptions, MENU_SIZE);

        if (choice >= 0 && choice < MENU_SIZE)
        {
            menu[choice]->action();
        }
        else
        {
            running = false;
            break;
        }
    }

    exit_command(EXIT_SUCCESS);

    return 0;
}

static char **bitmap_options()
{
    static char *options[BITMAP_COUNT] = {NULL};
    uint32_t i = 0;

    if (options[0] != NULL)
    {
        return options;
    }

    for (i = 0; i < BITMAP_COUNT; i++)
    {
        options[i] = get_allocated("Bitmap %" PRIu32, i + 1);

        if (options[i] == NULL)
        {
            goto cleanup;
        }
    }

    return options;

cleanup:

    for (i = 0; i < BITMAP_COUNT; i++)
    {
        free(options[i]);
        options[i] = NULL;
    }

    return options;
}

void handle_change_capacity(void)
{
    int32_t selected_index = 0;
    uint32_t new_capacity = 0;
    struct bitmap *bm_new = NULL;
    char *headers[HEADER_SIZE] = {NULL};

    headers[0] = "Choose Bitmap";
    selected_index = select_option(headers, HEADER_SIZE, bitmap_options(), BITMAP_COUNT);

    if (BETWEEN(selected_index, -1, BITMAP_COUNT))
    {
        new_capacity = get_int("Enter new capacity", 8, NULL);
        printf(CLEAR_SCREEN);
        fflush(stdout);

        if (new_capacity == UINT32_MAX)
        {
            printf("Invalid input.\n");
            goto cleanup;
        }

        if (new_capacity > UINT16_MAX)
        {
            new_capacity = UINT16_MAX;
            printf("Capacity exceeded, creating at max capacity (%" PRIu16 ").\n", UINT16_MAX);
        }
    }
    else
    {
        printf("Invalid bitmap selected.\n");
        goto cleanup;
    }

    bm_new = bitmap_create((u16)new_capacity);

    if (bm_new == NULL || !bitmap_or(bm_new, bitmaps[selected_index]))
    {
        printf("Failed to change capacity.\n");
        goto cleanup;
    }

    bitmap_destroy(bitmaps[selected_index]);
    bitmaps[selected_index] = NULL;
    bitmaps[selected_index] = bm_new;

    printf("Capacity Updated.\n");

cleanup:
    press_any_key();

    return;
}

void handle_add_value(void)
{
    int32_t selected_index = 0;
    uint32_t value = 0;
    char *headers[HEADER_SIZE] = {NULL};

    headers[0] = "Choose Bitmap";
    selected_index = select_option(headers, HEADER_SIZE, bitmap_options(), BITMAP_COUNT);

    if (BETWEEN(selected_index, -1, BITMAP_COUNT))
    {
        value = get_int("Enter value to add", 8, NULL);
        printf(CLEAR_SCREEN);
        fflush(stdout);

        if (value == UINT32_MAX)
        {
            printf("Invalid input.\n");
            goto cleanup;
        }

        if (value >= UINT16_MAX || !bitmap_add_value(bitmaps[selected_index], value))
        {
            printf("Failed to add %" PRIu32 " to Bitmap %" PRIu32 ".\n", value, selected_index + 1);
            goto cleanup;
        }
    }
    else
    {
        printf("Invalid bitmap selected.\n");
        goto cleanup;
    }

    printf("Added %" PRIu32 " to Bitmap %" PRIu32, value, selected_index + 1);

cleanup:
    press_any_key();

    return;
}

void handle_del_value(void)
{
    int32_t selected_index = 0;
    uint32_t value = 0;
    char *headers[HEADER_SIZE] = {NULL};

    headers[0] = "Choose Bitmap";

    selected_index = select_option(headers, HEADER_SIZE, bitmap_options(), BITMAP_COUNT);

    if (BETWEEN(selected_index, -1, BITMAP_COUNT))
    {
        value = get_int("Enter value to delete", 8, NULL);
        printf(CLEAR_SCREEN);
        fflush(stdout);

        if (value == UINT32_MAX)
        {
            printf("Invalid input.\n");
            goto cleanup;
        }

        if (value < UINT16_MAX && !bitmap_del_value(bitmaps[selected_index], value))
        {
            printf("Failed to delete %" PRIu32 " from Bitmap %" PRIu32 ".\n", value,
                   selected_index + 1);
            goto cleanup;
        }
    }
    else
    {
        printf("Invalid bitmap selected.\n");
        goto cleanup;
    }

    printf("Deleted %" PRIu32 " from Bitmap %" PRIu32, value, selected_index + 1);

cleanup:
    press_any_key();

    return;
}

void handle_invert_bitmap(void)
{
    int32_t selected_index = 0;
    char *headers[HEADER_SIZE] = {NULL};

    headers[0] = "Choose Bitmap";
    selected_index = select_option(headers, HEADER_SIZE, bitmap_options(), BITMAP_COUNT);

    if (BETWEEN(selected_index, -1, BITMAP_COUNT))
    {
        if (!bitmap_not(bitmaps[selected_index]))
        {
            printf("Failed to add invert Bitmap %" PRIu32 ".\n", selected_index + 1);
            goto cleanup;
        }
    }
    else
    {
        printf("Invalid bitmap selected.\n");
        goto cleanup;
    }

    printf("Inverted Bitmap %" PRIu32, selected_index + 1);

cleanup:
    press_any_key();

    return;
}

void handle_or_bitmap(void)
{
    int32_t index_store = 0;
    int32_t index_2nd = 0;
    char *headers1[HEADER_SIZE] = {NULL};
    char *headers2[HEADER_SIZE] = {NULL};

    headers1[0] = "Choose destination Bitmap";
    headers2[0] = "Choose second Bitmap";

    index_store = select_option(headers1, HEADER_SIZE, bitmap_options(), BITMAP_COUNT);
    index_2nd = select_option(headers2, HEADER_SIZE, bitmap_options(), BITMAP_COUNT);

    if (BETWEEN(index_store, -1, BITMAP_COUNT) && BETWEEN(index_2nd, -1, BITMAP_COUNT))
    {
        if (!bitmap_or(bitmaps[index_store], bitmaps[index_2nd]))
        {
            printf("Failed to OR Bitmap %" PRIu32 " and Bitmap %" PRIu32 ".\n", index_store + 1,
                   index_2nd + 1);
            goto cleanup;
        }
    }
    else
    {
        printf("Invalid bitmap selected.\n");
        goto cleanup;
    }

    printf("OR operation successful.\n");

cleanup:
    press_any_key();

    return;
}

void handle_and_bitmap(void)
{
    int32_t index_store = 0;
    int32_t index_2nd = 0;
    char *headers1[HEADER_SIZE] = {NULL};
    char *headers2[HEADER_SIZE] = {NULL};

    headers1[0] = strdup("Choose destination Bitmap");
    headers2[0] = strdup("Choose second Bitmap");

    index_store = select_option(headers1, HEADER_SIZE, bitmap_options(), BITMAP_COUNT);
    index_2nd = select_option(headers2, HEADER_SIZE, bitmap_options(), BITMAP_COUNT);

    if (BETWEEN(index_store, -1, BITMAP_COUNT) && BETWEEN(index_2nd, -1, BITMAP_COUNT))
    {
        if (!bitmap_and(bitmaps[index_store], bitmaps[index_2nd]))
        {
            printf("Failed to AND Bitmap %" PRIu32 " and Bitmap %" PRIu32 ".\n", index_store + 1,
                   index_2nd + 1);
            goto cleanup;
        }
    }
    else
    {
        printf("Invalid bitmap selected.\n");
        goto cleanup;
    }

    printf("AND Operation successful.\n");

cleanup:
    press_any_key();

    return;
}

void handle_parse_bitmap(void)
{
    int32_t selected_index = 0;
    char *input_str = NULL;
    struct bitmap *parsed_bm = NULL;
    char *headers[HEADER_SIZE] = {NULL};

    headers[0] = "Choose Bitmap";
    selected_index = select_option(headers, HEADER_SIZE, bitmap_options(), BITMAP_COUNT);

    if (BETWEEN(selected_index, -1, BITMAP_COUNT))
    {
        input_str = get_raw_str("Enter bitmap string (e.g., 1-3,5,7)", MAX_INPUT_SIZE);
        parsed_bm = bitmap_parse_str((u8 *)input_str);
        free(input_str);
        input_str = NULL;

        if (parsed_bm == NULL)
        {
            printf("Failed to parse bitmap string.\n");
            goto cleanup;
        }
        else
        {
            bitmap_destroy(bitmaps[selected_index]);
            bitmaps[selected_index] = parsed_bm;
        }
    }
    else
    {
        printf("Invalid bitmap selected.\n");
        goto cleanup;
    }

    printf("Parsing successful.\n");
    printf("Parsed bitmap: ");
    bitmap_print(bitmaps[selected_index]);

cleanup:
    press_any_key();

    return;
}

void handle_print_bitmap(void)
{
    int32_t i = 0;

    printf(CLEAR_SCREEN);

    for (i = 0; i < BITMAP_COUNT; i++)
    {
        printf("Bitmap %d: ", i + 1);
        bitmap_print(bitmaps[i]);
    }

    press_any_key();

    return;
}

void handle_clone_bitmap(void)
{
    int32_t index_store = 0;
    int32_t index_2nd = 0;
    char *headers1[HEADER_SIZE] = {NULL};
    char *headers2[HEADER_SIZE] = {NULL};
    struct bitmap *bm_temp = NULL;

    headers1[0] = "Choose destination Bitmap";
    headers2[0] = "Choose source Bitmap";

    index_store = select_option(headers1, HEADER_SIZE, bitmap_options(), BITMAP_COUNT);
    index_2nd = select_option(headers2, HEADER_SIZE, bitmap_options(), BITMAP_COUNT);

    if (index_store < BITMAP_COUNT && index_2nd < BITMAP_COUNT)
    {
        bm_temp = bitmap_clone(bitmaps[index_2nd]);

        if (bm_temp == NULL)
        {
            printf("Failed to clone Bitmap %" PRIu32 " into Bitmap %" PRIu32 ".\n", index_2nd + 1,
                   index_store + 1);
            goto cleanup;
        }

        bitmap_destroy(bitmaps[index_store]);
        bitmaps[index_store] = NULL;
        bitmaps[index_store] = bm_temp;
    }
    else
    {
        printf("Invalid bitmap selected.\n");
        goto cleanup;
    }

    printf("Cloning successful.\n");

cleanup:
    press_any_key();

    return;
}

void cleanup_bitmaps(void)
{
    exit_command(EXIT_SUCCESS);
}

char *get_allocated(const char *format, ...)
{
    char *buffer = NULL;
    int buffer_length = 0;
    va_list args;

    va_start(args, format);

    /* Calculate required buffer size */
    buffer_length = vsnprintf(NULL, 0, format, args);

    if (buffer_length < 0)
    {
        goto cleanup;
    }

    buffer_length++;

    /* Allocate memory */
    buffer = (char *)malloc(buffer_length * sizeof(char));

    if (buffer == NULL)
    {
        goto cleanup;
    }

    /* Fill the buffer with the formatted string */
    va_start(args, format);
    vsnprintf(buffer, buffer_length, format, args);

cleanup:
    va_end(args);

    return buffer;
}
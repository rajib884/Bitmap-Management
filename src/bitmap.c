#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitmap.h"

#ifdef DEBUG
    #define debug(fmt, ...) printf("%s:%d %s => " fmt, __FILE__, __LINE__, __func__, ##__VA_ARGS__);
#else
    #define debug(fmt, ...)
#endif

#define BITSIZEOF(type) (CHAR_BIT * sizeof(type))
#define CHAR_SPACE ' '
#define CHAR_NULL '\0'
#define CHAR_ENTRY_SEPARATOR ','
#define CHAR_RANGE_SEPARATOR '-'

static inline u8 *skip_space(u8 *str);

/*****************************************************************************
 *
 *   Name:       bitmap_check
 *
 *   Input:      bm          A bitmap that need to check
 *   Return:     Success     true
 *               Failed      false
 *   Description            Check a bitmap whether it's exist
 ******************************************************************************/
static bool bitmap_check(struct bitmap *bm);

/*****************************************************************************
 *
 *   Name:       update_info
 *
 *   Input:      bm          A bitmap that need to check
 *   Return:     Success     None
 *               Failed      None
 *   Description            Update bitmap first_value, last_value and numbers
 ******************************************************************************/
static void update_info(struct bitmap *bm);

static inline u8 *skip_space(u8 *str)
{
    if (str == NULL)
    {
        return NULL;
    }

    while (*str == CHAR_SPACE)
    {
        str++;
    }

    return str;
}

static void update_info(struct bitmap *bm)
{
    u16 i = 0;

    if (bm == NULL)
    {
        return;
    }

    bm->first_value = UINT16_MAX;
    bm->last_value = 0;
    bm->numbers = 0;

    for (i = 0; i < bm->max_value; i++)
    {
        if (bm->buf[i / BITSIZEOF(u32)] & (1U << (i % BITSIZEOF(u32))))
        {
            bm->numbers++;

            if (i < bm->first_value)
            {
                bm->first_value = i;
            }
            if (i > bm->last_value)
            {
                bm->last_value = i;
            }
        }
    }

    return;
}

struct bitmap *bitmap_create(u16 capacity)
{
    u16 buf_len = 0;
    size_t size_of_bitmap = 0;
    struct bitmap *bm = NULL;

    if (capacity == 0)
    {
        return NULL;
    }

    buf_len = capacity / BITSIZEOF(u32);

    if (buf_len * BITSIZEOF(u32) < capacity)
    {
        buf_len++;
    }

    size_of_bitmap = sizeof(struct bitmap) + buf_len * sizeof(u32);
    bm = (struct bitmap *)malloc(size_of_bitmap);

    if (bm == NULL)
    {
        return NULL;
    }

    bm->bm_self = bm;
    bm->max_value = capacity;
    bm->first_value = UINT16_MAX;
    bm->last_value = 0;
    bm->numbers = 0;
    bm->buf_len = buf_len;

    memset(bm->buf, 0, buf_len * sizeof(u32));

    return bm;
}

void bitmap_destroy(struct bitmap *bm)
{
    if (bm == NULL)
    {
        return;
    }

    bm->bm_self = NULL;

    free(bm);
    bm = NULL;

    return;
}

static bool bitmap_check(struct bitmap *bm)
{
    if (bm == NULL)
    {
        return false;
    }

    if (bm->bm_self != bm)
    {
        return false;
    }

    if (bm->max_value == 0 || bm->buf_len == 0)
    {
        return false;
    }

    if (bm->numbers > bm->max_value)
    {
        return false;
    }

    if (bm->buf_len * BITSIZEOF(u32) < bm->max_value)
    {
        return false;
    }

    return true;
}

bool bitmap_add_value(struct bitmap *bm, u16 value)
{
    u16 index = 0;
    u16 bit_position = 0;

    if (!bitmap_check(bm) || value >= bm->max_value)
    {
        return false;
    }

    index = value / BITSIZEOF(u32);
    bit_position = value % BITSIZEOF(u32);

    if (bm->buf[index] & (1U << bit_position))
    {
        debug("Bit already set at %" PRIu16 "\n", value);
        return true;
    }

    bm->buf[index] |= (1U << bit_position);

    if (value < bm->first_value)
    {
        bm->first_value = value;
    }

    if (value > bm->last_value)
    {
        bm->last_value = value;
    }

    bm->numbers++;

    debug("Bit set at %d\n", value);

    return true;
}

bool bitmap_del_value(struct bitmap *bm, u16 value)
{
    u16 index = 0;
    u16 bit_position = 0;

    if (!bitmap_check(bm) || value >= bm->max_value)
    {
        return false;
    }

    index = value / BITSIZEOF(u32);
    bit_position = value % BITSIZEOF(u32);

    if ((bm->buf[index] & (1U << bit_position)) == 0)
    {
        debug("Bit already reset at %" PRIu16 "\n", value);
        return true;
    }

    bm->buf[index] &= ~(1U << bit_position);

    update_info(bm); /* Recalculate info from buffer */

    debug("Bit reset at %d\n", value);

    return true;
}

void bitmap_print(struct bitmap *bm)
{
    bool in_range = false;
    u16 range_start = 0;
    u16 range_end = 0;
    u16 i = 0;

    if (!bitmap_check(bm))
    {
        printf("Invalid Bitmap\n");
        return;
    }

    if (bm->numbers == 0)
    {
        printf("No values\n");
        goto debug_print;
    }

    in_range = false;

    for (i = bm->first_value; i <= bm->last_value; i++)
    {
        if (bm->buf[i / BITSIZEOF(u32)] & (1U << (i % BITSIZEOF(u32))))
        {
            if (!in_range)
            {
                /* Start of new range */
                range_start = i;
                in_range = true;
            }
        }
        else
        {
            if (in_range)
            {
                /* End of the current range */
                range_end = i - 1;

                if (range_start == range_end)
                {
                    printf("%u", range_start);
                }
                else
                {
                    printf("%u%c%u", range_start, CHAR_RANGE_SEPARATOR, range_end);
                }

                in_range = false;

                if (i < bm->last_value)
                {
                    putchar(CHAR_ENTRY_SEPARATOR);
                }
            }
        }
    }

    if (in_range)
    {
        /* last range extends to last_value */
        if (range_start == bm->last_value)
        {
            printf("%u", range_start);
        }
        else
        {
            printf("%u%c%u", range_start, CHAR_RANGE_SEPARATOR, bm->last_value);
        }
    }

    printf("\n");

debug_print:

#ifdef DEBUG
    printf("More Info:\n");
    printf("  bm->max_value: %" PRIu16 "\n", bm->max_value);
    printf("  bm->first_value: %" PRIu16 "\n", bm->first_value);
    printf("  bm->last_value: %" PRIu16 "\n", bm->last_value);
    printf("  bm->numbers: %" PRIu16 "\n", bm->numbers);
    printf("  bm->buf_len: %" PRIu16 "\n", bm->buf_len);
    printf("Hex: ");
    i = bm->buf_len;

    while (i-- > 0)
    {
        printf("%04x ", bm->buf[i]);
    }
    printf("\n");

    printf("Bin: ");
    i = bm->buf_len;

    while (i-- > 0)
    {
        printf("%032b ", bm->buf[i]);
    }
    printf("\n\n");
#endif

    return;
}

struct bitmap *bitmap_clone(struct bitmap *bm)
{
    size_t size_of_bitmap = 0;
    struct bitmap *new_bm = NULL;

    if (!bitmap_check(bm))
    {
        return NULL;
    }

    /* Allocate memory for the new bitmap structure */
    size_of_bitmap = sizeof(struct bitmap) + bm->buf_len * sizeof(u32);
    new_bm = (struct bitmap *)malloc(size_of_bitmap);

    if (new_bm == NULL)
    {
        return NULL;
    }

    memcpy(new_bm, bm, size_of_bitmap);
    new_bm->bm_self = new_bm;

    return new_bm;
}

bool bitmap_not(struct bitmap *bm)
{
    u32 i = 0;
    u32 num_bits_in_last_buf = 0;

    if (!bitmap_check(bm))
    {
        return false;
    }

    /* Invert all bits in place */
    for (i = 0; i <= bm->max_value / BITSIZEOF(u32); i++)
    {
        bm->buf[i] = ~bm->buf[i];
    }

    /* undo invert last few extra bits */
    num_bits_in_last_buf = (bm->max_value % BITSIZEOF(u32));
    bm->buf[bm->max_value / BITSIZEOF(u32)] &= (1u << num_bits_in_last_buf) - 1;

    update_info(bm); /* Recalculate info from buffer */

    return true;
}

bool bitmap_or(struct bitmap *bm_store, struct bitmap *bm)
{
    u16 i = 0;
    u16 min_buffer_len = 0;
    u32 num_bits_in_last_buf = 0;

    if (!bitmap_check(bm_store) || !bitmap_check(bm))
    {
        return false;
    }

    /* OR only upto minimum size of the two bitmap */
    min_buffer_len = (bm_store->buf_len < bm->buf_len) ? bm_store->buf_len : bm->buf_len;

    for (i = 0; i < min_buffer_len; i++)
    {
        bm_store->buf[i] |= bm->buf[i];
    }

    /* clear last few extra bits, if any */
    num_bits_in_last_buf = (bm_store->max_value % BITSIZEOF(u32));
    bm_store->buf[bm_store->max_value / BITSIZEOF(u32)] &= (1u << num_bits_in_last_buf) - 1;

    update_info(bm_store); /* Recalculate info from buffer */

    return true;
}

bool bitmap_and(struct bitmap *bm_store, struct bitmap *bm)
{
    u16 i = 0;
    u16 min_buffer_len = 0;
    u32 num_bits_in_last_buf = 0;

    if (!bitmap_check(bm_store) || !bitmap_check(bm))
    {
        return false;
    }

    min_buffer_len = (bm_store->buf_len < bm->buf_len) ? bm_store->buf_len : bm->buf_len;

    for (i = 0; i < min_buffer_len; i++)
    {
        bm_store->buf[i] &= bm->buf[i];
    }

    /* clear last few extra bits, if any */
    num_bits_in_last_buf = (bm_store->max_value % BITSIZEOF(u32));
    bm_store->buf[bm_store->max_value / BITSIZEOF(u32)] &= (1u << num_bits_in_last_buf) - 1;

    /* clear rest of the buffer, if any */
    while (i < bm_store->buf_len)
    {
        bm_store->buf[i] = 0;
        i++;
    }

    update_info(bm_store); /* Recalculate info from buffer */

    return true;
}

struct bitmap *bitmap_parse_str(u8 *str)
{
    u8 *startptr = NULL;
    u8 *endptr = NULL;
    long value = 0;
    long max_value = 0;
    u16 start_value = 0;
    bool in_range = false;
    struct bitmap *bm = NULL;

    if (str == NULL || *str == CHAR_NULL)
    {
        return NULL;
    }

    debug("str: %s\n", str);

    /* Find max value, also if the string contains invalid characters */
    startptr = str;
    max_value = -1;

    while (*startptr != CHAR_NULL)
    {
        if (*startptr == CHAR_SPACE || *startptr == CHAR_RANGE_SEPARATOR ||
            *startptr == CHAR_ENTRY_SEPARATOR)
        {
            startptr++;
            continue;
        }

        if (isdigit(*startptr))
        {
            errno = 0;
            value = strtol((char *)startptr, (char **)&endptr, 10);

            if (value >= UINT16_MAX || value < 0 || errno == ERANGE)
            {
                debug("Out of range\n");
                goto cleanup;
            }

            if (value > max_value)
            {
                max_value = value;
            }

            startptr = endptr;
        }
        else
        {
            debug("Invalid character in string\n");
            goto cleanup;
        }
    }

    if (max_value < 0)
    {
        debug("No value in string\n");
        goto cleanup;
    }

    debug("Max val in str: %ld\n", max_value);
    bm = bitmap_create((u16)max_value + 1);

    if (bm == NULL)
    {
        debug("call to bitmap_create failed\n");
        goto cleanup;
    }

    /* Parse the string and set bits */
    start_value = 0;
    startptr = str;
    endptr = NULL;
    in_range = false;

    while (*startptr != CHAR_NULL)
    {
        startptr = skip_space(startptr);

        if (*startptr == CHAR_NULL)
        {
            break;
        }

        if (*startptr == CHAR_RANGE_SEPARATOR)
        {
            goto cleanup;
        }

        errno = 0;
        value = strtol((char *)startptr, (char **)&endptr, 10);

        if (value >= UINT16_MAX || value < 0 || errno == ERANGE)
        {
            debug("This should have been caught in the first iteration,"
                  " but somehow we found a value that is Out of range");
            goto cleanup;
        }

        if (value == 0 && endptr == startptr)
        {
            goto cleanup;
        }

        startptr = skip_space(endptr);

        if (*startptr == CHAR_ENTRY_SEPARATOR || *startptr == CHAR_NULL)
        {
            if (in_range)
            {
                if (start_value > value)
                {
                    debug("Invalid range: Range start is less than range end!\n");
                    goto cleanup;
                }

                while (start_value <= value)
                {
                    bitmap_add_value(bm, start_value);
                    start_value++;
                }
                in_range = false;
            }
            else
            {
                bitmap_add_value(bm, (u16)value);
            }

            if (*startptr == CHAR_ENTRY_SEPARATOR)
            {
                startptr++;
            }
        }
        else if (*startptr == CHAR_RANGE_SEPARATOR && !in_range)
        {
            /* this was first value of range */
            debug("Range started\n");
            in_range = true;
            start_value = (u16)value;
            startptr++;
        }
        else
        {
            debug("Invalid string.\n");
            goto cleanup;
        }
    }

    if (in_range)
    {
        goto cleanup;
    }

    return bm;

cleanup:
    debug("Invalid string \n");
    bitmap_destroy(bm);
    bm = NULL;

    return NULL;
}

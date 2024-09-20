#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

struct bitmap
{
    struct bitmap *bm_self;
    u16 max_value;   /* The value used when creating a bitmap, aka capacity */
    u16 first_value; /* The first bit has been set */
    u16 last_value;  /* The last bit has been set */
    u16 numbers;     /* numbers of '1' in buf[] */
    u16 buf_len;
    u32 buf[0];
};

/*****************************************************************************
 *
 *   Name:       bitmap_create
 *
 *   Input:      capacity    The capacity of the bitmap that will be created
 *   Return:     Success     bitmap
 *               Failed      NULL
 *   Description            Create a bitmap
 ******************************************************************************/
struct bitmap *bitmap_create(u16 capacity);

/*****************************************************************************
 *
 *   Name:       bitmap_destroy
 *
 *   Input:      bm          A bitmap that will be destroyed
 *   Return:     Success     None
 *               Failed      remind
 *   Description            Destory a bitmap
 ******************************************************************************/
void bitmap_destroy(struct bitmap *bm);

/*****************************************************************************
 *
 *   Name:       bitmap_add_value
 *
 *   Input:      bm          The bitmap to which values are added
 *               value       A value that will be added into the bitmap
 *   Return:     Success     true
 *               Failed      false
 *   Description            Add a value into the bitmap
 ******************************************************************************/
bool bitmap_add_value(struct bitmap *bm, u16 value);

/*****************************************************************************
 *
 *   Name:       bitmap_del_value
 *
 *   Input:      bm          The bitmap to which values are removed
 *               value       A value that will be removed from the specified bitmap
 *   Return:     Success     true
 *               Failed      false
 *   Description            Remove a value from the specified bitmap
 ******************************************************************************/
bool bitmap_del_value(struct bitmap *bm, u16 value);

/*****************************************************************************
 *
 *   Name:       bitmap_print
 *
 *   Input:      bm          A bitmap that will be printed
 *   Return:     Success     None
 *               Failed      None
 *   Description            Print all the elements in the bitmap
 ******************************************************************************/
void bitmap_print(struct bitmap *bm);

/*****************************************************************************
 *
 *   Name:       bitmap_clone
 *
 *   Input:      bm          A bitmap that will be copied
 *   Return:     Success     None
 *               Failed      None
 *   Description            Create a new bitmap and copy specified bm data into it
 ******************************************************************************/
struct bitmap *bitmap_clone(struct bitmap *bm);

/*****************************************************************************
 *
 *   Name:       bitmap_not
 *
 *   Input:      bm          A bitmap that will be reversed with all the binary bits
 *   Return:     Success     None
 *               Failed      None
 *   Description            Reverse all the binary bits
 ******************************************************************************/
bool bitmap_not(struct bitmap *bm);

/*****************************************************************************
 *
 *   Name:       bitmap_or
 *
 *   Input:      bm_store    A bitmap that participates in binary or operations and
 *                           stores the results
 *               bm          Another bitmap that participates in binary or operations
 *   Return:     Success     true
 *               Failed      false
 *   Description            bm_store | bm
 ******************************************************************************/
bool bitmap_or(struct bitmap *bm_store, struct bitmap *bm);

/*****************************************************************************
 *
 *   Name:       bitmap_and
 *
 *   Input:      bm_store    A bitmap that participates in binary and operations and
 *                           stores the results
 *               bm          Another bitmap that participates in binary and operations
 *   Return:     Success     true
 *               Failed      false
 *   Description            bm_store & bm
 ******************************************************************************/
bool bitmap_and(struct bitmap *bm_store, struct bitmap *bm);

/*****************************************************************************
 *
 *   Name:       bitmap_parse_str
 *
 *   Input:      str         A string that will be parsed to a bitmap
 *   Return:     Success     A bitmap that stores the data from the specified string
 *               Failed      None
 *   Description            Parse a string to bitmap
 ******************************************************************************/
struct bitmap *bitmap_parse_str(u8 *str);

#endif /* __BITMAP_H__ */
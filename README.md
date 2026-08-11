## Bitmap Management

A C bitmap library, with an interactive terminal front end for exercising it.
A bitmap stores a set of `uint16_t` values as bits, so membership, union and
intersection cost a word operation each rather than a scan.

The bitmap is a single allocation. `struct bitmap` carries the capacity, the
first and last values set and the population count, and the bit buffer is a
flexible array member at the end of the same block — one `malloc`, one `free`,
and the header and its data can never be separated. Every entry point first
checks a self-pointer stored inside the structure, so a freed or corrupted
handle is rejected instead of followed.

### Library

| Function | Description |
| --- | --- |
| `bitmap_create` / `bitmap_destroy` | Allocate and release a bitmap of a given capacity |
| `bitmap_add_value` / `bitmap_del_value` | Set or clear one bit, maintaining the cached first, last and population values |
| `bitmap_and` / `bitmap_or` / `bitmap_not` | Bitwise intersection, union and complement, applied in place |
| `bitmap_clone` | Copy a bitmap into a new allocation |
| `bitmap_parse_str` | Build a bitmap from a range string such as `1-3,5,7` |
| `bitmap_print` | Print the values a bitmap currently holds |

### Usage

1. **Compile the Code:** Use option `-DUSE_UNICODE` if your terminal supports
   printing unicode characters.

   ```bash
   gcc main.c src/*.c -Iinclude -o main -DUSE_UNICODE
   ```

   `make` does the same thing.

2. **Run the Program:**

   ```bash
   ./main
   ```

   The menu holds five bitmaps at once. You can add and delete values, change
   a bitmap's capacity, clone one, invert it, and AND or OR any two of them
   into a chosen destination.

The front end puts the terminal into raw mode through `termios`, so it builds
on Linux, macOS and WSL rather than natively on Windows.

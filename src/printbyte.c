#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define COL_SIZE 8

typedef struct
{
    const char *str;
    unsigned int n;
} my_t;

my_t *s1[] = {&(my_t){"╔", 1},    &(my_t){"═", 4 + sizeof(void *) * 2},
              &(my_t){"╤═", 1},   &(my_t){"═", COL_SIZE * 3},
              &(my_t){"╤═", 1},   &(my_t){"═", COL_SIZE},
              &(my_t){"═╗\n", 1}, NULL};
my_t *s2[] = {&(my_t){"╟", 1},    &(my_t){"─", 4 + sizeof(void *) * 2},
              &(my_t){"┼─", 1},   &(my_t){"─", COL_SIZE * 3},
              &(my_t){"┼─", 1},   &(my_t){"─", COL_SIZE},
              &(my_t){"─╢\n", 1}, NULL};
my_t *s3[] = {&(my_t){"╚", 1},    &(my_t){"═", 4 + sizeof(void *) * 2},
              &(my_t){"╧═", 1},   &(my_t){"═", COL_SIZE * 3},
              &(my_t){"╧═", 1},   &(my_t){"═", COL_SIZE},
              &(my_t){"═╝\n", 1}, NULL};

static void helper(my_t *ss[])
{
    unsigned int i;
    for (; *ss != NULL; ss++)
        for (i = 0; i < ((*ss)->n); i++)
            fputs((*ss)->str, stdout);
}

void print_data(const void *start, size_t size, char print_type)
{
    const unsigned char *data = (const unsigned char *)start;
    size_t i;
    unsigned int lst_col;
    char *lbl, *exchar;

    if (print_type == 'b')
    {
        lbl = "Binary";
        exchar = "         ";
        lst_col = COL_SIZE * 9;
    }
    else if (print_type == 'd')
    {
        lbl = "Decimal";
        exchar = "    ";
        lst_col = COL_SIZE * 4;
    }
    else
    {
        lbl = "ASCII";
        exchar = " ";
        lst_col = COL_SIZE;
    }
    (*s1[5]).n = lst_col;
    (*s2[5]).n = lst_col;
    (*s3[5]).n = lst_col;

    helper(s1);

    printf("║ %-*.*s │ %-*.*s │ %-*.*s ║\n", (int)(2 + sizeof(void *) * 2),
           (int)(2 + sizeof(void *) * 2), "Address", COL_SIZE * 3 - 1, COL_SIZE * 3 - 1,
           "Hexadecimal Values", (int)lst_col, (int)lst_col, lbl);

    helper(s2);

    for (i = 0; i < size; i += COL_SIZE)
    {
        printf("║ 0x%0*zx │ ", (int)(sizeof(void *) * 2), (size_t)(data + i));

        // Print hexadecimal values
        size_t j = 0;
        for (; j < COL_SIZE && i + j < size; ++j)
            printf("%02x ", data[i + j]);

        for (; j < COL_SIZE; ++j)
            printf("   ");

        // Print custom representation
        printf("│ ");
        if (print_type == 'b')
        {
            for (j = 0; j < COL_SIZE && i + j < size; ++j)
            {
                printf("%08b ", data[i + j]);
            }
        }
        else if (print_type == 'd')
        {
            for (j = 0; j < COL_SIZE && i + j < size; ++j)
            {
                printf("%3d ", data[i + j]);
            }
        }
        else
        {
            for (j = 0; j < COL_SIZE && i + j < size; ++j)
            {
                if (isprint(data[i + j]))
                {
                    printf("%c", data[i + j]);
                }
                else if (data[i + j] == '\0')
                {
                    printf(".");
                }
                else
                {
                    printf("▪");
                }
            }
        }

        for (; j < COL_SIZE; ++j)
            printf(exchar);

        printf(" ║\n");
    }
    helper(s3);
}

void print_ascii(const void *start, size_t size)
{
    print_data(start, size, '\0');
}

// int main(void) {
//     int a = 0xAF;
//     char cc[15] = "abcdefgh";
//     cc[10] = 'a';
//     int b = 0xFA;
//     print_data(cc, 100);
//     return 0;
// }

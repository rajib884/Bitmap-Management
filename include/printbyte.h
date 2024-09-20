#ifndef __PRINTBYTE_H__
#define __PRINTBYTE_H__

#include <stddef.h>

void print_data(const void *start, size_t size, char print_type);
void print_ascii(const void *start, size_t size);

#endif /* __PRINTBYTE_H__*/
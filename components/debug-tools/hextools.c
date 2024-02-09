#include "hextools.h"
#include <stdint.h>
#include <stdio.h>

// Bin 2 hex printer.
void rawprinthex(uint64_t val, int digits) {
    const char hextab[] = "0123456789ABCDEF";
    for (; digits > 0; digits--) {
        putc(hextab[(val >> (digits * 4 - 4)) & 15], stdout);
    }
}

#define LOGK_HEXDUMP_COLS   16
#define LOGK_HEXDUMP_GROUPS 4
// Print a hexdump, override the address shown (usually for debug purposes).
void hexdump_vaddr(const char* msg, const void* data, size_t size, size_t vaddr) {
    fputs(msg, stdout);
    putc('\r', stdout);
    putc('\n', stdout);

    const uint8_t* ptr = data;
    for (size_t y = 0; y * LOGK_HEXDUMP_COLS < size; y++) {
        rawprinthex((size_t)vaddr + y * LOGK_HEXDUMP_COLS, sizeof(size_t) * 2);
        putc(':', stdout);
        size_t x;
        for (x = 0; y * LOGK_HEXDUMP_COLS + x < size && x < LOGK_HEXDUMP_COLS; x++) {
            if ((x % LOGK_HEXDUMP_GROUPS) == 0) {
                putc(' ', stdout);
            }
            putc(' ', stdout);
            rawprinthex(ptr[y * LOGK_HEXDUMP_COLS + x], 2);
        }
        for (; x < LOGK_HEXDUMP_GROUPS; x++) {
            if ((x % LOGK_HEXDUMP_GROUPS) == 0) {
                putc(' ', stdout);
            }
            putc(' ', stdout);
            putc(' ', stdout);
            putc(' ', stdout);
        }
        putc(' ', stdout);
        putc(' ', stdout);
        for (x = 0; y * LOGK_HEXDUMP_COLS + x < size && x < LOGK_HEXDUMP_COLS; x++) {
            char c = (char)ptr[y * LOGK_HEXDUMP_COLS + x];
            if (c >= 0x20 && c <= 0x7e) {
                putc(c, stdout);
            } else {
                putc('.', stdout);
            }
        }
        putc('\r', stdout);
        putc('\n', stdout);
    }
    putc('\r', stdout);
    putc('\n', stdout);
}

void hexdump(const char* msg, const void* data, size_t size) {
    hexdump_vaddr(msg, data, size, (size_t)data);
}

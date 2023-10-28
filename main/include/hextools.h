#pragma once

#include <stdint.h>
#include <stdio.h>


void hexdump_vaddr(char const *msg, void const *data, size_t size, size_t vaddr);
void hexdump(char const *msg, void const *data, size_t size);

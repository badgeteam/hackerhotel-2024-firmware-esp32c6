#pragma once

#include <stdint.h>
#include <stdio.h>


void hexdump_vaddr(const char* msg, const void* data, size_t size, size_t vaddr);
void hexdump(const char* msg, const void* data, size_t size);

#pragma once
#include <stdint.h>

struct limine_framebuffer;

void fb_console_init(void *address, uint64_t width, uint64_t height, uint64_t pitch, uint16_t bpp);
void fb_console_putc(char c);
void fb_console_write(const char *str);
void kprintf(const char *fmt, ...);
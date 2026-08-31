#include "fb_console.h"
#include "font8x8.h"
#include "spinlock.h"
#include <stdarg.h>

static uint8_t *fb_addr;
static uint64_t fb_width, fb_height, fb_pitch;
static uint16_t fb_bpp;

static uint32_t cursor_x = 0, cursor_y = 0;
static const uint32_t GLYPH_W = 8, GLYPH_H = 8;
static const uint32_t LINE_HEIGHT = 12;
static const uint32_t FG_COLOR = 0xffffffff;
static const uint32_t BG_COLOR = 0xff000000;

static Spinlock console_lock;

void fb_console_init(void *address, uint64_t width, uint64_t height, uint64_t pitch, uint16_t bpp) {
    fb_addr = (uint8_t *)address;
    fb_width = width;
    fb_height = height;
    fb_pitch = pitch;
    fb_bpp = bpp;
    cursor_x = 0;
    cursor_y = 0;
    spinlock_init(&console_lock);
}

static inline void put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (x >= fb_width || y >= fb_height) return;
    uint32_t *row = (uint32_t *)(fb_addr + y * fb_pitch);
    row[x] = color;
}

static void draw_glyph(uint32_t px, uint32_t py, char c) {
    uint8_t uc = (uint8_t)c;
    if (uc < 0x20 || uc > 0x7F) uc = '?';
    const uint8_t* glyph = font8x8_basic[uc - 0x20];

    for (uint32_t row = 0; row < GLYPH_H; row++) {
        uint8_t bits = glyph[row];
        for (uint32_t col = 0; col < GLYPH_W; col++) {
            bool on = (bits >> col) & 1;
            put_pixel(px + col, py + row, on ? FG_COLOR : BG_COLOR);
        }
    }
}

static void scroll() {
    uint32_t bytes_per_row = fb_pitch;
    uint8_t* dst = fb_addr;
    uint8_t* src = fb_addr + LINE_HEIGHT * bytes_per_row;
    uint64_t bytes_to_move = (fb_height - LINE_HEIGHT) * bytes_per_row;

    for (uint64_t i = 0; i < bytes_to_move; i++) {
        dst[i] = src[i];
    }

    uint8_t* clear_start = fb_addr + (fb_height - LINE_HEIGHT) * bytes_per_row;
    for (uint64_t i = 0; i < LINE_HEIGHT * bytes_per_row; i++) {
        clear_start[i] = 0x00;
    }
}

void fb_console_putc(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y += LINE_HEIGHT;
    } else if (c == '\r') {
        cursor_x = 0;
    } else {
        draw_glyph(cursor_x, cursor_y, c);
        cursor_x += GLYPH_W;
        if (cursor_x + GLYPH_W > fb_width) {
            cursor_x = 0;
            cursor_y += LINE_HEIGHT;
        }
    }

    if (cursor_y + LINE_HEIGHT > fb_height) {
        scroll();
        cursor_y -= LINE_HEIGHT;
    }
}

void fb_console_write(const char *str) {
    while (*str) fb_console_putc(*str++);
}

static void print_uint(uint64_t val, int base, bool uppercase) {
    char buf[32];
    const char *digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
    int i = 0;

    if (val == 0) {
        fb_console_putc('0');
        return;
    }

    while (val > 0) {
        buf[i++] = digits[val % base];
        val /= base;
    }

    while (i > 0) fb_console_putc(buf[--i]);
}

static void print_int(int64_t val) {
    if (val < 0) {
        fb_console_putc('-');
        print_uint((uint64_t)(-val), 10, false);
    } else {
        print_uint((uint64_t)val, 10, false);
    }
}

void kprintf(const char *fmt, ...) {
    spinlock_acquire(&console_lock);

    va_list args;
    va_start(args, fmt);

    for (const char *p = fmt; *p; p++) {
        if (*p != '%') {
            fb_console_putc(*p);
            continue;
        }

        p++;
        switch (*p) {
            case 'd': print_int(va_arg(args, int)); break;
            case 'u': print_uint(va_arg(args, unsigned int), 10, false); break;
            case 'x': print_uint(va_arg(args, unsigned int), 16, false); break;
            case 'X': print_uint(va_arg(args, unsigned int), 16, true); break;
            case 'p':
                fb_console_write("0x");
                print_uint((uint64_t)va_arg(args, void *), 16, false);
                break;
            case 's': fb_console_write(va_arg(args, const char*)); break;
            case 'c': fb_console_putc((char)va_arg(args, int)); break;
            case '%': fb_console_putc('%'); break;
            default:
                fb_console_putc('%');
                fb_console_putc(*p);
        }
    }

    va_end(args);
    spinlock_release(&console_lock);
}
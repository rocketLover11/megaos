#include "pmm.h"
#include "fb_console.h"

#define PAGE_SIZE 4096

struct limine_memmap_entry {
    uint64_t base;
    uint64_t length;
    uint64_t type;
};

#define MEMMAP_USABLE 0

static uint8_t *bitmap = nullptr;
static uint64_t bitmap_size_bytes = 0;
static uint64_t total_pages = 0;
static uint64_t free_pages = 0;
static uint64_t hhdm_off = 0;
static uint64_t last_alloc_index = 0;

static inline void bitmap_set(uint64_t bit) {
    bitmap[bit / 8] |= (1 << (bit % 8));
}

static inline void bitmap_clear(uint64_t bit) {
    bitmap[bit / 8] &= ~(1 << (bit % 8));
}

static inline bool bitmap_test(uint64_t bit) {
    return bitmap[bit / 8] & (1 << (bit % 8));
}

void pmm_init(limine_memmap_entry **entries, uint64_t entry_count, uint64_t hhdm_offset) {
    hhdm_off = hhdm_offset;

    uint64_t highest_addr = 0;

    for (uint64_t i = 0; i < entry_count; i++) {
        limine_memmap_entry *e = entries[i];
        uint64_t end = e->base + e->length;
        if (end > highest_addr) highest_addr = end;
    }

    total_pages = highest_addr / PAGE_SIZE;
    bitmap_size_bytes = (total_pages + 7) / 8;

    for (uint64_t i = 0; i < entry_count; i++) {
        limine_memmap_entry *e = entries[i];
        if (e->type == MEMMAP_USABLE && e->length >= bitmap_size_bytes) {
            bitmap = (uint8_t *)(e->base + hhdm_off);
            break;
        }
    }

    if (!bitmap) {
        kprintf("[megakernel] No usable region big enough for bitmap!\n");
        for (;;) asm("hlt");
    }

    for (uint64_t i = 0; i < bitmap_size_bytes; i++) {
        bitmap[i] = 0xff;
    }

    free_pages = 0;

    for (uint64_t i = 0; i < entry_count; i++) {
        limine_memmap_entry *e = entries[i];
        if (e->type != MEMMAP_USABLE) continue;

        uint64_t start_page = e->base / PAGE_SIZE;
        uint64_t page_count = e->length / PAGE_SIZE;

        for (uint64_t p = 0; p < page_count; p++) {
            bitmap_clear(start_page + p);
            free_pages++;
        }
    }

    uint64_t bitmap_phys_base = (uint64_t)bitmap - hhdm_off;
    uint64_t bitmap_start_page = bitmap_phys_base / PAGE_SIZE;
    uint64_t bitmap_page_count = (bitmap_size_bytes + PAGE_SIZE - 1) / PAGE_SIZE;

    for (uint64_t p = 0; p < bitmap_page_count; p++) {
        uint64_t bit = bitmap_start_page + p;
        if (!bitmap_test(bit)) {
            bitmap_set(bit);
            free_pages--;
        }
    }

    kprintf("[megakernel] PMM initialized.");
}

uint64_t pmm_alloc_page() {
    for (uint64_t i = last_alloc_index; i < total_pages; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            free_pages--;
            last_alloc_index = i;
            return i * PAGE_SIZE;
        }
    }

    return 0;
}

void pmm_free_page(uint64_t phys_addr) {
    uint64_t bit = phys_addr / PAGE_SIZE;
    if (bit >= total_pages) return;
    if (bitmap_test(bit)) {
        bitmap_clear(bit);
        free_pages++;
    }
}

uint64_t pmm_get_free_pages() { return free_pages; }
uint64_t pmm_get_total_pages() { return total_pages; }
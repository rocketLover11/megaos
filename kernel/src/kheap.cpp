#include "kheap.h"
#include "pmm.h"
#include "vmm.h"
#include "fb_console.h"

#define PAGE_SIZE 4096

#define HEAP_VIRT_BASE 0xffffa00000000000ULL
#define HEAP_INITIAL_SIZE (64 * 1024)

struct BlockHeader {
    size_t size;
    bool free;
    BlockHeader *addr_next;
    BlockHeader *addr_prev;
    BlockHeader *free_next;
};

static BlockHeader *free_list_head = nullptr;
static BlockHeader *block_list_head = nullptr;
static uint64_t heap_current_top = HEAP_VIRT_BASE;

static const size_t ALIGNMENT = 16;

static inline size_t align_up(size_t n, size_t align) {
    return (n + align - 1) & ~(align - 1);
}

static void heap_expand(size_t min_size) {
    size_t needed = align_up(min_size + sizeof(BlockHeader), PAGE_SIZE);
    size_t pages = needed / PAGE_SIZE;

    uint64_t new_block_virt = heap_current_top;

    for (size_t i = 0; i < pages; i++) {
        uint64_t phys = pmm_alloc_page();
        if (phys == 0) {
            kprintf("[megakernel] Out of physical memory expanding heap!\n");
            for (;;) asm("hlt");
        }
        vmm_map(heap_current_top, phys, VMM_WRITABLE | VMM_NX);
        heap_current_top += PAGE_SIZE;
    }

    BlockHeader *new_block = (BlockHeader *)new_block_virt;
    new_block->size = (pages * PAGE_SIZE) - sizeof(BlockHeader);
    new_block->free = true;
    new_block->addr_next = nullptr;
    new_block->addr_prev = nullptr;
    new_block->free_next = nullptr;

    if (!block_list_head) {
        block_list_head = new_block;
    } else {
        BlockHeader *last = block_list_head;
        while (last->addr_next) last = last->addr_next;
        last->addr_next = new_block;
        new_block->addr_prev = last;

        uint64_t prev_end = (uint64_t)last + sizeof(BlockHeader) + last->size;
        if (last->free && prev_end == (uint64_t)new_block) {
            last->size += sizeof(BlockHeader) + new_block->size;
            last->addr_next = new_block->addr_next;
            new_block = last;
        }
    }

    new_block->free_next = free_list_head;
    free_list_head = new_block;
}

void kheap_init() {
    heap_expand(HEAP_INITIAL_SIZE);
    kprintf("[megakernel] Initialized heap with %d bytes.\n", (int)HEAP_INITIAL_SIZE);
}

static void remove_from_free_list(BlockHeader *target) {
    if (free_list_head == target) {
        free_list_head = target->free_next;
        return;
    }
    BlockHeader *cur = free_list_head;
    while (cur && cur->free_next != target) cur = cur->free_next;
    if (cur) cur->free_next = target->free_next;
}

void *kmalloc(size_t size) {
    if (size == 0) return nullptr;
    size = align_up(size, ALIGNMENT);

    BlockHeader *best = nullptr;
    BlockHeader *cur = free_list_head;

    while (cur) {
        if (cur->free && cur->size >= size) {
            best = cur;
            break;
        }
        cur = cur->free_next;
    }

    if (!best) {
        heap_expand(size);
        cur = free_list_head;
        while (cur) {
            if (cur->free && cur->size >= size) {
                best = cur;
                break;
            }
            cur = cur->free_next;
        }
        if (!best) {
            kprintf("[megakernel] Allocation failed after expand!\n");
            return nullptr;
        }
    }

    size_t remaining = best->size - size;
    if (remaining > sizeof(BlockHeader) + ALIGNMENT) {
        BlockHeader *split = (BlockHeader *)((uint8_t *)best + sizeof(BlockHeader) + size);
        split->size = remaining - sizeof(BlockHeader);
        split->free = true;
        split->addr_next = best->addr_next;
        split->addr_prev = best;
        if (best->addr_next) best->addr_next->addr_prev = split;
        best->addr_next = split;

        split->free_next = best->free_next;
        best->free_next = split;

        best->size = size;
    }

    remove_from_free_list(best);
    best->free = false;

    return (void *)((uint8_t *)best + sizeof(BlockHeader));
}

void kfree(void *ptr) {
    if (!ptr) return;

    BlockHeader *block = (BlockHeader *)((uint8_t *)ptr - sizeof(BlockHeader));
    block->free = true;

    if (block->addr_next && block->addr_next->free) {
        BlockHeader *next = block->addr_next;
        remove_from_free_list(next);
        block->size += sizeof(BlockHeader) + next->size;
        block->addr_next = next->addr_next;
        if (next->addr_next) next->addr_next->addr_prev = block;
    }

    if (block->addr_prev && block->addr_prev->free) {
        BlockHeader *prev = block->addr_prev;
        remove_from_free_list(prev);
        prev->size += sizeof(BlockHeader) + block->size;
        prev->addr_next = block->addr_next;
        if (block->addr_next) block->addr_next->addr_prev = prev;
        block = prev;
    }

    block->free_next = free_list_head;
    free_list_head = block;
}
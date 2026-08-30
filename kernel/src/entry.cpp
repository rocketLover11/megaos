#include <stdint.h>
#include <stddef.h>

#include "fb_console.h"
#include "gdt.h"
#include "idt.h"
#include "pmm.h"
#include "vmm.h"
#include "kheap.h"

extern "C" {

__attribute__((used, section(".requests")))
static volatile struct {
    uint64_t id[4];
    uint64_t revision;
    struct limine_framebuffer_response *response;
} framebuffer_request = {
    .id = {0xc7b1dd30df4c8b88, 0x0a82e883a194f07b, 0x9d5827dcd881dd75, 0xa3148604f6fab11b},
    .revision = 0,
    .response = nullptr
};

struct limine_memmap_entry {
    uint64_t base;
    uint64_t length;
    uint64_t type;
};

struct limine_memmap_response {
    uint64_t revision;
    uint64_t entry_count;
    struct limine_memmap_entry **entries;
};

__attribute__((used, section(".requests")))
static volatile struct {
    uint64_t id[4];
    uint64_t revision;
    struct limine_memmap_response *response;
} memmap_request = {
    .id = {0xc7b1dd30df4c8b88, 0x0a82e883a194f07b, 0x67cf3d9d378a806f, 0xe304acdfc50c3c62},
    .revision = 0,
    .response = nullptr
};

struct limine_hhdm_response {
    uint64_t revision;
    uint64_t offset;
};

__attribute__((used, section(".requests")))
static volatile struct {
    uint64_t id[4];
    uint64_t revision;
    struct limine_hhdm_response *response;
} hhdm_request = {
    .id = {0xc7b1dd30df4c8b88, 0x0a82e883a194f07b, 0x48dcf1cb8ad2b852, 0x63984e959a98244b},
    .revision = 0,
    .response = nullptr
};

struct limine_kernel_address_response {
    uint64_t revision;
    uint64_t physical_base;
    uint64_t virtual_base;
};

__attribute__((used, section(".requests")))
static volatile struct {
    uint64_t id[4];
    uint64_t revision;
    struct limine_kernel_address_response *response;
} kernel_address_request = {
    .id = {0xc7b1dd30df4c8b88, 0x0a82e883a194f07b, 0x71ba76863cc55f63, 0xb2644a48c516a487},
    .revision = 0,
    .response = nullptr
};

struct limine_framebuffer {
    void *address;
    uint64_t width;
    uint64_t height;
    uint64_t pitch;
    uint16_t bpp;
    uint8_t memory_model;
    uint8_t red_mask_size;
    uint8_t red_mask_shift;
    uint8_t green_mask_size;
    uint8_t green_mask_shift;
    uint8_t blue_mask_size;
    uint8_t blue_mask_shift;
    uint8_t unused[7];
    uint64_t edid_size;
    void *edid;
};

struct limine_framebuffer_response {
    uint64_t revision;
    uint64_t framebuffer_count;
    struct limine_framebuffer **framebuffers;
};

} // extern "C"

static void hcf() {
    for (;;) {
        asm ("hlt");
    }
}

extern "C" void kmain() {
    if (framebuffer_request.response == nullptr ||
        framebuffer_request.response->framebuffer_count < 1) {
        hcf();
    }

    struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];

    if (fb->bpp != 32) {
        hcf();
    }

    fb_console_init(fb->address, fb->width, fb->height, fb->pitch, fb->bpp);

    gdt_init();
    kprintf("[megakernel] GDT loaded.\n");

    idt_init();
    kprintf("[megakernel] IDT loaded.\n");

    if (memmap_request.response == nullptr || hhdm_request.response == nullptr) {
        kprintf("[megakernel] Missing memmap or hhdm response!\n");
        hcf();
    }

    pmm_init(memmap_request.response->entries, memmap_request.response->entry_count, hhdm_request.response->offset);

    extern char __kernel_end;
    vmm_init(hhdm_request.response->offset, memmap_request.response->entries, memmap_request.response->entry_count, kernel_address_request.response->physical_base, kernel_address_request.response->virtual_base, (uint64_t)&__kernel_end, (uint64_t)fb->address - hhdm_request.response->offset, fb->pitch * fb->height);

    kheap_init();

    hcf();
}
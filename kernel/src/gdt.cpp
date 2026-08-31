#include "gdt.h"

struct GDTEntry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

struct TSSDescriptorHigh {
    uint32_t base_upper;
    uint32_t reserved;
} __attribute__((packed));

struct GDTPointer {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static GDTEntry gdt[7];
static GDTPointer gdt_ptr;

extern "C" void gdt_flush(uint64_t gdt_ptr_addr);

static void gdt_set_entry(int idx, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[idx].base_low = base & 0xffff;
    gdt[idx].base_mid = (base >> 16) & 0xff;
    gdt[idx].base_high = (base >> 24) & 0xff;
    gdt[idx].limit_low = limit & 0xffff;
    gdt[idx].granularity = ((limit >> 16) & 0x0f) | (gran & 0xf0);
    gdt[idx].access = access;
}

void gdt_set_tss_descriptor(uint64_t base, uint32_t limit) {
    gdt_set_entry(5, (uint32_t)(base & 0xffffffff), limit, 0x89, 0x00);

    TSSDescriptorHigh *high = (TSSDescriptorHigh *)&gdt[6];
    high->base_upper = (uint32_t)(base >> 32);
    high->reserved = 0;
}

void gdt_init() {
    gdt_ptr.limit = sizeof(gdt) - 1;
    gdt_ptr.base = (uint64_t)&gdt;

    gdt_set_entry(0, 0, 0, 0, 0);
    gdt_set_entry(1, 0, 0xfffff, 0x9a, 0xaf);
    gdt_set_entry(2, 0, 0xfffff, 0x92, 0xcf);
    gdt_set_entry(3, 0, 0xfffff, 0xfa, 0xaf);
    gdt_set_entry(4, 0, 0xfffff, 0xf2, 0xcf);

    gdt_flush((uint64_t)&gdt_ptr);
}
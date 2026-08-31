#include "tss.h"

struct TSS {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iopb_offset;
} __attribute__((packed));

static TSS tss;

extern "C" void tss_flush(uint16_t selector);

void tss_init(uint64_t kernel_stack_top) {
    for (uint8_t *p = (uint8_t *)&tss; p < (uint8_t *)&tss + sizeof(TSS); p++) *p = 0;

    tss.rsp0 = kernel_stack_top;
    tss.iopb_offset = sizeof(TSS);

    extern void gdt_set_tss_descriptor(uint64_t base, uint32_t limit);
    gdt_set_tss_descriptor((uint64_t)&tss, sizeof(TSS) - 1);

    tss_flush(0x28);
}

void tss_set_kernel_stack(uint64_t stack_top) {
    tss.rsp0 = stack_top;
}
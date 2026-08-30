#pragma once
#include <stdint.h>
#include <stddef.h>

struct limine_memmap_entry;

#define VMM_PRESENT     (1ULL << 0)
#define VMM_WRITABLE    (1ULL << 1)
#define VMM_USER        (1ULL << 2)
#define VMM_NX          (1ULL << 63)

void vmm_init(uint64_t hhdm_offset, limine_memmap_entry **entries, uint64_t entry_count, uint64_t kernel_phys_base, uint64_t kernel_virt_base, uint64_t kernel_end_virt, uint64_t fb_phys_base, uint64_t fb_size);
void vmm_map(uint64_t virt, uint64_t phys, uint64_t flags);
void vmm_unmap(uint64_t virt);
uint64_t vmm_get_kernel_pml4_phys();
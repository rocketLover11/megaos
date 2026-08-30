#include "vmm.h"
#include "pmm.h"
#include "fb_console.h"

#define PAGE_SIZE 4096
#define ENTRIES_PER_TABLE 512
#define PTE_ADDR_MASK 0x000ffffffffff000ULL

struct limine_memmap_entry {
    uint64_t base;
    uint64_t length;
    uint64_t type;
};

#define MEMMAP_USABLE 0
#define MEMMAP_BOOTLOADER_RECLAIMABLE 5

static uint64_t hhdm_off = 0;
static uint64_t kernel_pml4_phys = 0;

static inline uint64_t *phys_to_virt(uint64_t phys) {
    return (uint64_t *)(phys + hhdm_off);
}

static uint64_t *get_or_create_table(uint64_t *parent_table, uint64_t index, bool create) {
    uint64_t entry = parent_table[index];

    if (entry & VMM_PRESENT) {
        return phys_to_virt(entry & PTE_ADDR_MASK);
    }

    if (!create) return nullptr;

    uint64_t new_table_phys = pmm_alloc_page();
    if (new_table_phys == 0) {
        kprintf("[megakernel] out of memory allocating page table\n");
        for (;;) asm("hlt");
    }

    uint64_t *new_table_virt = phys_to_virt(new_table_phys);
    for (int i = 0; i < ENTRIES_PER_TABLE; i++) new_table_virt[i] = 0;

    parent_table[index] = new_table_phys | VMM_PRESENT | VMM_WRITABLE;
    return new_table_virt;
}

static void map_page_in(uint64_t *pml4, uint64_t virt, uint64_t phys, uint64_t flags) {
    uint64_t pml4_idx = (virt >> 39) & 0x1ff;
    uint64_t pdpt_idx = (virt >> 30) & 0x1ff;
    uint64_t pd_idx   = (virt >> 21) & 0x1ff;
    uint64_t pt_idx   = (virt >> 12) & 0x1ff;
    
    uint64_t *pdpt = get_or_create_table(pml4, pml4_idx, true);
    uint64_t *pd   = get_or_create_table(pdpt, pdpt_idx, true);
    uint64_t *pt   = get_or_create_table(pd, pd_idx, true);

    pt[pt_idx] = (phys & PTE_ADDR_MASK) | flags | VMM_PRESENT;
}

static void map_range(uint64_t *pml4, uint64_t virt_start, uint64_t phys_start, uint64_t size, uint64_t flags) {
    uint64_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    for (uint64_t i = 0; i < pages; i++) {
        map_page_in(pml4, virt_start + i * PAGE_SIZE, phys_start + i * PAGE_SIZE, flags);
    }
}

void vmm_init(uint64_t hhdm_offset, limine_memmap_entry **entries, uint64_t entry_count, uint64_t kernel_phys_base, uint64_t kernel_virt_base, uint64_t kernel_end_virt, uint64_t fb_phys_base, uint64_t fb_size) {
    hhdm_off = hhdm_offset;

    kernel_pml4_phys = pmm_alloc_page();
    uint64_t *pml4 = phys_to_virt(kernel_pml4_phys);
    for (int i = 0; i < ENTRIES_PER_TABLE; i++) pml4[i] = 0;

    for (uint64_t i = 0; i < entry_count; i++) {
        limine_memmap_entry *e = entries[i];
        if (e->type != MEMMAP_USABLE && e->type != MEMMAP_BOOTLOADER_RECLAIMABLE) continue;

        uint64_t aligned_base = e->base & ~(PAGE_SIZE - 1);
        uint64_t aligned_len = e->length + (e->base - aligned_base);

        map_range(pml4, hhdm_off + aligned_base, aligned_base, aligned_len, VMM_WRITABLE | VMM_NX);
    }

    uint64_t kernel_size = kernel_end_virt - kernel_virt_base;
    map_range(pml4, kernel_virt_base, kernel_phys_base, kernel_size, VMM_WRITABLE);

    uint64_t fb_aligned_base = fb_phys_base & ~(PAGE_SIZE - 1);
    uint64_t fb_aligned_len = fb_size + (fb_phys_base - fb_aligned_base);
    map_range(pml4, hhdm_off + fb_aligned_base, fb_aligned_base, fb_aligned_len, VMM_WRITABLE | VMM_NX);

    asm volatile ("mov %0, %%cr3" : : "r"(kernel_pml4_phys) : "memory");

    kprintf("[megakernel] VMM initialized, new PML4 at %p, CR3 switched\n", (void *)kernel_pml4_phys);
}

void vmm_map(uint64_t virt, uint64_t phys, uint64_t flags) {
    uint64_t *pml4 = phys_to_virt(kernel_pml4_phys);
    map_page_in(pml4, virt, phys, flags);
    asm volatile ("invlpg (%0)" : : "r"(virt) : "memory");
}

void vmm_unmap(uint64_t virt) {
    uint64_t *pml4 = phys_to_virt(kernel_pml4_phys);

    uint64_t pml4_idx = (virt >> 39) & 0x1FF;
    uint64_t pdpt_idx = (virt >> 30) & 0x1FF;
    uint64_t pd_idx   = (virt >> 21) & 0x1FF;
    uint64_t pt_idx   = (virt >> 12) & 0x1FF;

    uint64_t *pdpt = get_or_create_table(pml4, pml4_idx, false);
    if (!pdpt) return;
    uint64_t *pd = get_or_create_table(pdpt, pdpt_idx, false);
    if (!pd) return;
    uint64_t *pt = get_or_create_table(pd, pd_idx, false);
    if (!pt) return;

    pt[pt_idx] = 0;
    asm volatile ("invlpg (%0)" : : "r"(virt) : "memory");
}

uint64_t vmm_get_kernel_pml4_phys() {
    return kernel_pml4_phys;
}
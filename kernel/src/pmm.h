#pragma once
#include <stdint.h>
#include <stddef.h>

struct limine_memmap_entry;

void pmm_init(limine_memmap_entry **entries, uint64_t entry_count, uint64_t hhdm_offset);
uint64_t pmm_alloc_page();
void pmm_free_page(uint64_t phys_addr);
uint64_t pmm_get_free_pages();
uint64_t pmm_get_total_pages();
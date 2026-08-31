#pragma once
#include <stdint.h>

void tss_init(uint64_t kernel_stack_top);
void tss_set_kernel_stack(uint64_t stack_top);
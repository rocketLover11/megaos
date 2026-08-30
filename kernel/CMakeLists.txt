add_executable(kernel
    src/entry.cpp
    src/font8x8.cpp
    src/fb_console.cpp
    src/gdt.cpp
    src/gdt_flush.s
    src/idt.cpp
    src/isr.s
    src/idt_load.s
    src/pmm.cpp
    src/vmm.cpp
    src/kheap.cpp
)

set_source_files_properties(
    src/gdt_flush.s
    src/isr.s
    src/idt_load.s
    PROPERTIES LANGUAGE ASM_NASM
)

target_compile_options(kernel PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-ffreestanding>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-stack-protector>
    $<$<COMPILE_LANGUAGE:CXX>:-mno-red-zone>
    $<$<COMPILE_LANGUAGE:CXX>:-mcmodel=kernel>
    $<$<COMPILE_LANGUAGE:CXX>:-Wall>
    $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
)

target_link_options(kernel PRIVATE
    -nostdlib
    -static
    -ffreestanding
    -no-pie
    -Wl,-T,${CMAKE_CURRENT_SOURCE_DIR}/linker.ld
)

set_target_properties(kernel PROPERTIES
    LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld
)
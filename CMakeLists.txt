cmake_minimum_required(VERSION 3.20)
project(megaos ASM_NASM CXX)

enable_language(ASM_NASM)

add_subdirectory(kernel)

set(ISO_ROOT ${CMAKE_BINARY_DIR}/iso_root)
set(LIMINE_DIR ${CMAKE_SOURCE_DIR}/limine)

add_custom_target(iso
    COMMAND ${CMAKE_COMMAND} -E make_directory ${ISO_ROOT}/boot/limine
    COMMAND ${CMAKE_COMMAND} -E make_directory ${ISO_ROOT}/EFI/BOOT
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:kernel> ${ISO_ROOT}/boot/kernel.elf
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/limine.conf ${ISO_ROOT}/boot/limine/limine.conf
    COMMAND ${CMAKE_COMMAND} -E copy ${LIMINE_DIR}/limine-bios.sys ${ISO_ROOT}/boot/limine/
    COMMAND ${CMAKE_COMMAND} -E copy ${LIMINE_DIR}/limine-bios-cd.bin ${ISO_ROOT}/boot/limine/
    COMMAND ${CMAKE_COMMAND} -E copy ${LIMINE_DIR}/limine-uefi-cd.bin ${ISO_ROOT}/boot/limine/
    COMMAND ${CMAKE_COMMAND} -E copy ${LIMINE_DIR}/BOOTX64.EFI ${ISO_ROOT}/EFI/BOOT/
    COMMAND xorriso -as mkisofs -R -r -J -b boot/limine/limine-bios-cd.bin
            -no-emul-boot -boot-load-size 4 -boot-info-table
            --efi-boot boot/limine/limine-uefi-cd.bin
            -efi-boot-part --efi-boot-image --protective-msdos-label
            ${ISO_ROOT} -o ${CMAKE_BINARY_DIR}/megaos.iso
    COMMAND ${LIMINE_DIR}/limine bios-install ${CMAKE_BINARY_DIR}/megaos.iso
    DEPENDS kernel
    COMMENT "Building bootable ISO"
)

add_custom_target(run
    COMMAND qemu-system-x86_64 -cdrom ${CMAKE_BINARY_DIR}/megaos.iso -m 512M
            -serial stdio -no-reboot -no-shutdown -d int,cpu_reset -D qemu.log
    DEPENDS iso
)
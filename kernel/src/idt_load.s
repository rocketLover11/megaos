global idt_load
section .text
bits 64

idt_load:
    lidt [rdi]
    ret
global tss_flush
section .text
bits 64

tss_flush:
    ltr di
    ret
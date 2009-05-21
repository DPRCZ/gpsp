@ vim:filetype=armasm

.global gp2x_flush_cache @ beginning_addr, end_addr, flags

gp2x_flush_cache:
    swi #0x9f0002
    mov pc, lr


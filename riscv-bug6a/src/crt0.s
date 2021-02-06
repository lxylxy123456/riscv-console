.section .init, "ax"
.global _start
_start:
    mv a1, a2
    jal _start

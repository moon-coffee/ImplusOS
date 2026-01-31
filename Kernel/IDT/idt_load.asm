BITS 64
global load_idt

load_idt:
    mov rax,rdi
    lidt [rax]
    ret

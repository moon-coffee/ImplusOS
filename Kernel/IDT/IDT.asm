BITS 64

global load_idt
global isr_default

extern serial_write_string

SECTION .data
isr_msg: db "[ISR] Default handler called!", 0x0A, 0

SECTION .text

isr_default:
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    
    lea rdi, [rel isr_msg]
    call serial_write_string
    
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    
    iretq

load_idt:
    cli
    lidt [rdi]
    sti
    ret
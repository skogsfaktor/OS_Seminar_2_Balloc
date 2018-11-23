global start
extern myos_main

bits 32

section .text ;This is the code 
start: 
    cli       ;Disables ?  
    mov esp, stack ;32 bit mode, moves a stack address to esp
    call myos_main ;Calls the main procedure in myos.c
    hlt

section .bss ;Sector that creates an 8k stack. Cant call c procedures without a stack

resb 8192   ;Allocates 8k of memory 
stack:
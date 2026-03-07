.CODE

PUBLIC DummyInterruptHandler
DummyInterruptHandler PROC
    iretq
DummyInterruptHandler ENDP

; void LoadTaskRegister(uint16_t selector)
PUBLIC LoadTaskRegister
LoadTaskRegister PROC
    ltr cx
    ret
LoadTaskRegister ENDP

; void SwitchStackAndCall(uintptr_t newStack, void* parameter, void (*function)(void*))
PUBLIC SwitchStackAndCall
SwitchStackAndCall PROC
    and rcx, -16
    mov rsp, rcx
    sub rsp, 32
    mov rcx, rdx
    call r8
    int 3
SwitchStackAndCall ENDP

END

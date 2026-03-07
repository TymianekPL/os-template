.686P
.MODEL FLAT

.CODE

PUBLIC DummyInterruptHandler
DummyInterruptHandler PROC
    iret
DummyInterruptHandler ENDP

; void LoadTaskRegister(uint16_t selector)
PUBLIC _LoadTaskRegister
_LoadTaskRegister PROC
    mov ax, [esp+4]
    ltr ax
    ret
_LoadTaskRegister ENDP

; void SwitchStackAndCall(uintptr_t newStack, void* parameter, void (*function)(void*))
PUBLIC SwitchStackAndCall
SwitchStackAndCall PROC
    mov eax, [esp+4]
    mov edx, [esp+8]
    mov ecx, [esp+12]
    and eax, -16
    mov esp, eax
    push edx
    call ecx
    int 3
SwitchStackAndCall ENDP

END

.global DummyInterruptHandler
.global _DummyInterruptHandler
DummyInterruptHandler:
_DummyInterruptHandler:
    iret

# void LoadTaskRegister(uint16_t selector)
.global _LoadTaskRegister
_LoadTaskRegister:
    mov 4(%esp), %ax
    ltr %ax
    ret

# void SwitchStackAndCall(uintptr_t newStack, void* parameter, void (*function)(void*))
.global SwitchStackAndCall
SwitchStackAndCall:
    mov 4(%esp), %eax
    mov 8(%esp), %edx
    mov 12(%esp), %ecx
    and $-16, %eax
    mov %eax, %esp
    push %edx
    call *%ecx
    int3



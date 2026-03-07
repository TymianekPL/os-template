.global DummyInterruptHandler
DummyInterruptHandler:
    iretq

# void LoadTaskRegister(uint16_t selector)
.global LoadTaskRegister
LoadTaskRegister:
    ltr %cx
    ret

# void SwitchStackAndCall(uintptr_t newStack, void* parameter, void (*function)(void*))
.global SwitchStackAndCall
SwitchStackAndCall:
    and $-16, %rcx
    mov %rcx, %rsp
    sub $32, %rsp
    mov %rdx, %rcx
    call *%r8
    int3

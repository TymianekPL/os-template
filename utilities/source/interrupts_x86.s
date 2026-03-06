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


